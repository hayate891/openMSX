#ifndef REVERSEMANGER_HH
#define REVERSEMANGER_HH

#include "Schedulable.hh"
#include "EventListener.hh"
#include "StateChangeListener.hh"
#include "Command.hh"
#include "EmuTime.hh"
#include "MemBuffer.hh"
#include "array_ref.hh"
#include <vector>
#include <map>
#include <memory>
#include <cstdint>

namespace openmsx {

class MSXMotherBoard;
class Keyboard;
class EventDelay;
class EventDistributor;
class TclObject;
class Interpreter;

class ReverseManager final : private EventListener, private StateChangeRecorder
{
public:
	ReverseManager(MSXMotherBoard& motherBoard);
	~ReverseManager();

	// Keyboard is special because we need to transfer the host keyboard
	// state on 'reverse goto' to be able to resynchronize when replay
	// stops. See Keyboard::transferHostKeyMatrix() for more info.
	void registerKeyboard(Keyboard& keyboard_) {
		keyboard = &keyboard_;
	}

	// To not loose any events we need to flush delayed events before
	// switching machine. See comments in goTo() for more info.
	void registerEventDelay(EventDelay& eventDelay_) {
		eventDelay = &eventDelay_;
	}

	// Should only be used by MSXMotherBoard to be able to transfer
	// reRecordCount to ReverseManager for version 2 of MSXMotherBoard
	// serializers.
	void setReRecordCount(unsigned count) {
		reRecordCount = count;
	}

private:
	struct ReverseChunk {
		ReverseChunk();
		ReverseChunk(ReverseChunk&& rhs);
		ReverseChunk& operator=(ReverseChunk&& rhs);

		EmuTime time;
		MemBuffer<uint8_t> savestate;

		// Number of recorded events (or replay index) when this
		// snapshot was created. So when going back replay should
		// start at this index.
		unsigned eventCount;
	};
	using Chunks = std::map<unsigned, ReverseChunk>;
	using Events = std::vector<std::shared_ptr<StateChange>>;

	struct ReverseHistory {
		void swap(ReverseHistory& other);
		void clear();
		unsigned getNextSeqNum(EmuTime::param time) const;

		Chunks chunks;
		Events events;
	};

	bool isCollecting() const { return collecting; }

	void start();
	void stop();
	void status(TclObject& result) const;
	void debugInfo(TclObject& result) const;
	void goBack(array_ref<TclObject> tokens);
	void goTo(array_ref<TclObject> tokens);
	void saveReplay(array_ref<TclObject> tokens, TclObject& result);
	void loadReplay(Interpreter& interp,
	                array_ref<TclObject> tokens, TclObject& result);

	void signalStopReplay(EmuTime::param time);
	EmuTime::param getEndTime(const ReverseHistory& history) const;
	void goTo(EmuTime::param targetTime, bool novideo);
	void goTo(EmuTime::param targetTime, bool novideo,
	          ReverseHistory& history, bool sameTimeLine);
	void transferHistory(ReverseHistory& oldHistory,
	                     unsigned oldEventCount);
	void transferState(MSXMotherBoard& newBoard);
	void takeSnapshot(EmuTime::param time);
	void schedule(EmuTime::param time);
	void replayNextEvent();
	template<unsigned N> void dropOldSnapshots(unsigned count);

	// Schedulable
	struct SyncBase : public Schedulable {
		SyncBase(Scheduler& s, ReverseManager& rm_)
			: Schedulable(s), rm(rm_) {}
		ReverseManager& rm;
		friend class ReverseManager;
	};
	struct SyncNewSnapshot : public SyncBase {
		SyncNewSnapshot(Scheduler& s, ReverseManager& m) : SyncBase(s, m) {}
		void executeUntil(EmuTime::param /*time*/) override {
			rm.execNewSnapshot();
		}
	} syncNewSnapshot;
	struct SyncInputEvent : public SyncBase {
		SyncInputEvent(Scheduler& s, ReverseManager& m) : SyncBase(s, m) {}
		void executeUntil(EmuTime::param /*time*/) override {
			rm.execInputEvent();
		}
	} syncInputEvent;

	void execNewSnapshot();
	void execInputEvent();
	EmuTime::param getCurrentTime() const { return syncNewSnapshot.getCurrentTime(); }

	// EventListener
	int signalEvent(const std::shared_ptr<const Event>& event) override;

	// StateChangeRecorder
	void signalStateChange(const std::shared_ptr<StateChange>& event) override;
	void stopReplay(EmuTime::param time) override;
	bool isReplaying() const override;

	MSXMotherBoard& motherBoard;
	EventDistributor& eventDistributor;

	class ReverseCmd final : public Command {
	public:
		ReverseCmd(ReverseManager& manager, CommandController& controller);
		void execute(array_ref<TclObject> tokens, TclObject& result) override;
		std::string help(const std::vector<std::string>& tokens) const override;
		void tabCompletion(std::vector<std::string>& tokens) const override;
	private:
		ReverseManager& manager;
	} reverseCmd;

	Keyboard* keyboard;
	EventDelay* eventDelay;
	ReverseHistory history;
	unsigned replayIndex;
	bool collecting;
	bool pendingTakeSnapshot;

	unsigned reRecordCount;

	friend struct Replay;
};

} // namespace openmsx

#endif
