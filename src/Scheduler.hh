// $Id$

#ifndef __SCHEDULER_HH__
#define __SCHEDULER_HH__

#include <SDL/SDL.h>
#include <set>
#include <string>
#include "EmuTime.hh"
#include "HotKey.hh"
#include "Mutex.hh"
#include "ConsoleSource/Command.hh"
#include "Schedulable.hh"
#include "Mutex.hh"

//forward declarations
class MSXCPU;


class Scheduler : private EventListener
{
	class SynchronizationPoint
	{
		public:
			SynchronizationPoint (const EmuTime &time, Schedulable *dev, int usrdat) :
				timeStamp(time), device(dev) , userData(usrdat) {}
			const EmuTime &getTime() const { return timeStamp; }
			Schedulable* getDevice() const { return device; }
			int getUserData() const { return userData; }
			bool operator< (const SynchronizationPoint &n) const
				{ return getTime() > n.getTime(); } // smaller time is higher priority
		private:
			EmuTime timeStamp;
			Schedulable* device;
			int userData;
	};

	public:
		/**
		 * Destructor
		 */
		virtual ~Scheduler();

		/**
		 * This is a singleton class,
		 *  usage: Scheduler::instance()->method();
		 */
		static Scheduler *instance();

		/**
		 * Register a syncPoint. When the emulation reaches "timestamp",
		 * the executeUntilEmuTime() method of "device" gets called.
		 * SyncPoints are ordered: smaller EmuTime -> scheduled
		 * earlier.
		 * If the supplied EmuTime may be smaller than the current CPU
		 * (normally an error). This is usefull if you want to schedule
		 * something ASAP, just pass a zero EmuTime.
		 * A device may register several syncPoints.
		 * Optionally a "userData" parameter can be passed, this
		 * parameter is not used by the Scheduler but it is passed to
		 * the executeUntilEmuTime() method of "device". This is useful
		 * if you want to distinguish between several syncPoint types.
		 * If you do not supply "userData" it is assumed to be zero.
		 */
		void setSyncPoint(const EmuTime &timestamp, Schedulable* device, int userData = 0);

		/**
		 * Removes a syncPoint of a given device that matches the given
		 * userData. Returns true if a match is found and removed.
		 * If there is more than one match only one will be removed,
		 * there is no guarantee that the earliest syncPoint is
		 * removed.
		 * Removing a syncPoint is a relatively expensive operation,
		 * if possible don't remove the syncPoint but ignore it in
		 * your executeUntilEmuTime() method.
		 */
		bool Scheduler::removeSyncPoint(Schedulable* device, int userdata = 0);

		/**
		 * This starts the schedule loop, should only be used by main
		 * the program.
		 */
		void scheduleEmulation();

		/**
		 * This stops the schedule loop, should only be used by the
		 * quit program routine.
		 */
		void stopScheduling();

		/**
		 * This pauses the emulation.
		 */
		void pause();

		/**
		 * This unpauses the emulation
		 */
		void unpause();

		/**
		 * True iff paused
		 */
		bool isPaused();

	private:
		Scheduler();
		void reschedule();

		// EventListener
		void signalEvent(SDL_Event &event);

		static Scheduler *oneInstance;
		/** Vector used as heap, not a priority queue because that
		  * doesn't allow removal of non-top element.
		  */
		std::vector<SynchronizationPoint> syncPoints;
		Mutex schedMutex;

		bool noSound;

		/** The scheduler is considered running unless it is paused
		  * or exited.
		  */
		bool runningScheduler;
		bool exitScheduler;
		bool paused;
		Mutex pauseMutex;
		MSXCPU *cpu;
		
		// commands
		class QuitCmd : public Command {
		public:
			virtual void execute(const std::vector<std::string> &tokens);
			virtual void help   (const std::vector<std::string> &tokens);
		};
		QuitCmd quitCmd;
		class MuteCmd : public Command {
		public:
			virtual void execute(const std::vector<std::string> &tokens);
			virtual void help   (const std::vector<std::string> &tokens);
		};
		friend class MuteCmd;
		MuteCmd muteCmd;
};

#endif
