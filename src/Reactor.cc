// $Id$

#include "Reactor.hh"
#include "MSXMotherBoard.hh"
#include "CommandLineParser.hh"
#include "CommandController.hh"
#include "Command.hh"
#include "Scheduler.hh"
#include "MSXCPU.hh"
#include "CliComm.hh"
#include "EventDistributor.hh"
#include "Display.hh"
#include "Timer.hh"
#include "GlobalSettings.hh"
#include "BooleanSetting.hh"
#include "FileContext.hh"
#include "FileException.hh"
#include <cassert>

using std::string;
using std::vector;

namespace openmsx {

class QuitCommand : public SimpleCommand
{
public:
	QuitCommand(CommandController& commandController, Reactor& reactor);
	virtual std::string execute(const std::vector<std::string>& tokens);
	virtual std::string help(const std::vector<std::string>& tokens) const;
private:
	Reactor& reactor;
};


Reactor::Reactor()
	: paused(false)
	, blockedCounter(0)
	, running(true)
	, pauseSetting(getMotherBoard().getCommandController().getGlobalSettings().
	                   getPauseSetting())
	, cliComm(getMotherBoard().getCliComm())
	, quitCommand(new QuitCommand(getMotherBoard().getCommandController(), *this))
{
	pauseSetting.attach(*this);

	getMotherBoard().getEventDistributor().registerEventListener(
		OPENMSX_QUIT_EVENT, *this);
}

Reactor::~Reactor()
{
	getMotherBoard().getEventDistributor().unregisterEventListener(
		OPENMSX_QUIT_EVENT, *this);

	pauseSetting.detach(*this);
}

MSXMotherBoard& Reactor::getMotherBoard()
{
	if (!motherBoard.get()) {
		motherBoard.reset(new MSXMotherBoard(*this));
	}
	return *motherBoard;
}

void Reactor::run(CommandLineParser& parser)
{
	CommandController& commandController(getMotherBoard().getCommandController());
	Display& display(getMotherBoard().getDisplay());

	// execute init.tcl
	try {
		SystemFileContext context(true); // only in system dir
		commandController.source(context.resolve("init.tcl"));
	} catch (FileException& e) {
		// no init.tcl, ignore
	}

	// execute startup scripts
	const CommandLineParser::Scripts& scripts = parser.getStartupScripts();
	for (CommandLineParser::Scripts::const_iterator it = scripts.begin();
	     it != scripts.end(); ++it) {
		try {
			UserFileContext context(commandController);
			commandController.source(context.resolve(*it));
		} catch (FileException& e) {
			throw FatalError("Couldn't execute script: " +
			                 e.getMessage());
		}
	}

	// Run
	if (parser.getParseStatus() == CommandLineParser::RUN) {
		// don't use TCL to power up the machine, we cannot pass
		// exceptions through TCL and ADVRAM might throw in its
		// powerUp() method. Solution is to implement dependencies
		// between devices so ADVRAM can check the error condition
		// in its constructor
		//commandController.executeCommand("set power on");
		getMotherBoard().powerUp();
	}
	
	Scheduler& scheduler(getMotherBoard().getScheduler());
	while (running) {
		bool blocked = blockedCounter > 0;
		if (!blocked) blocked = !getMotherBoard().execute();
		if (blocked) {
			display.repaint();
			Timer::sleep(100 * 1000);
			scheduler.doPoll();
			// TODO: Make Scheduler only responsible for events inside the MSX.
			//       All other events should be handled by the Reactor.
			scheduler.schedule(scheduler.getCurrentTime());
		}
	}
	getMotherBoard().doPowerDown(scheduler.getCurrentTime());
}

void Reactor::unpause()
{
	if (paused) {
		paused = false;
		cliComm.update(CliComm::STATUS, "paused", "false");
		unblock();
	}
}

void Reactor::pause()
{
	if (!paused) {
		paused = true;
		cliComm.update(CliComm::STATUS, "paused", "true");
		block();
	}
}

void Reactor::block()
{
	++blockedCounter;
	getMotherBoard().getCPU().exitCPULoop();
}

void Reactor::unblock()
{
	--blockedCounter;
	assert(blockedCounter >= 0);
}


// Observer<Setting>
void Reactor::update(const Setting& setting)
{
	if (&setting == &pauseSetting) {
		if (pauseSetting.getValue()) {
			pause();
		} else {
			unpause();
		}
	} else {
		assert(false);
	}
}

// EventListener
void Reactor::signalEvent(const Event& event)
{
	if (event.getType() == OPENMSX_QUIT_EVENT) {
		running = false;
		getMotherBoard().getCPU().exitCPULoop();
	} else {
		assert(false);
	}
}


// class QuitCommand
// TODO: Unify QuitCommand and OPENMSX_QUIT_EVENT.

QuitCommand::QuitCommand(CommandController& commandController,
                         Reactor& reactor_)
	: SimpleCommand(commandController, "exit")
	, reactor(reactor_)
{
}

string QuitCommand::execute(const vector<string>& /*tokens*/)
{
	reactor.running = false;
	reactor.getMotherBoard().getCPU().exitCPULoop();
	return "";
}

string QuitCommand::help(const vector<string>& /*tokens*/) const
{
	return "Use this command to stop the emulator\n";
}

} // namespace openmsx
