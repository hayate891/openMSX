// $Id$

#include "CassettePort.hh"
#include "CassetteDevice.hh"
#include "DummyCassetteDevice.hh"
#include "PluggingController.hh"
#include "MSXCPU.hh"
#include "CassettePlayer.hh"


// CassettePortFactory //

CassettePortInterface *CassettePortFactory::instance(const EmuTime &time)
{
	static CassettePortInterface* oneInstance = NULL;
	if (oneInstance == NULL) {
		try {
			MSXConfig::Backend::instance()->getConfigById("CassettePort");
			// there is a CassettePort in config
			oneInstance = new CassettePort(time);
		} catch (MSXConfig::Exception& e) {
			// there is no CassettePort in config
			oneInstance = new DummyCassettePort(time);
		}
	}
	return oneInstance;
}



// CassettePortInterface //

CassettePortInterface::CassettePortInterface(const EmuTime &time)
{
	dummy = new DummyCassetteDevice();
	pluggable = dummy;	//unplug(time);
}

CassettePortInterface::~CassettePortInterface()
{
	unplug(MSXCPU::instance()->getCurrentTime());
	delete dummy;
}

void CassettePortInterface::plug(Pluggable *dev, const EmuTime &time)
{
	Connector::plug(dev, time);
}

void CassettePortInterface::unplug(const EmuTime &time)
{
	flushOutput(time);
	Connector::unplug(time);
	plug(dummy, time);
}

const std::string &CassettePortInterface::getName()
{
	static const std::string name("cassetteport");
	return name;
}

const std::string &CassettePortInterface::getClass()
{
	static const std::string className("Cassette Port");
	return className;
}


// DummyCassettePort //

DummyCassettePort::DummyCassettePort(const EmuTime &time) : CassettePortInterface(time)
{
}

void DummyCassettePort::setMotor(bool status, const EmuTime &time)
{
	// do nothing
}
void DummyCassettePort::cassetteOut(bool output, const EmuTime &time)
{
	// do nothing
}
bool DummyCassettePort::cassetteIn(const EmuTime &time)
{
	return true;	// TODO check on Turbo-R
}
void DummyCassettePort::flushOutput(const EmuTime &time)
{
	// do nothing
}


// CassettePort //

CassettePort::CassettePort(const EmuTime &time) : CassettePortInterface(time)
{
	buffer = new short[BUFSIZE];
	PluggingController::instance()->registerConnector(this);

	player = new CassettePlayer();
}

CassettePort::~CassettePort()
{
	delete player;
	PluggingController::instance()->unregisterConnector(this);
	delete[] buffer;
}

void CassettePort::setMotor(bool status, const EmuTime &time)
{
	((CassetteDevice*)pluggable)->setMotor(status, time);
	//TODO make 'click' sound
}

void CassettePort::cassetteOut(bool output, const EmuTime &time)
{
	// this implements a VERY rough filter
	//   on a transition the output is 0
	//   everywhere else it is +A or -A
	// this is probably good enough
	//flushOutput(time);
	nextSample = 0;
	lastOutput = output;
}

bool CassettePort::cassetteIn(const EmuTime &time)
{
	// All analog filtering is ignored for now
	//   only important component is DC-removal
	//   we just assume sample has no DC component
	short sample = ((CassetteDevice*)pluggable)->readSample(time);	// read 1 sample  
	return (sample >= 0); // comparator
}

void CassettePort::flushOutput(const EmuTime &time)
{
	int sampleRate = ((CassetteDevice*)pluggable)->getWriteSampleRate();	// can be changed since prev flush
	if (sampleRate == 0) {
		// 99% of the time
		PRT_DEBUG("Cas: must not generate wave");
		prevTime = time;
		return;
	}
	int samples = (int)((time - prevTime).toFloat() * sampleRate);
	prevTime = time;
	PRT_DEBUG("Cas: generate " << samples << " samples");
	
	// dumb implementation, good enough for now
	((CassetteDevice*)pluggable)->writeWave(&nextSample, 1);
	samples--;
	nextSample = lastOutput ? 32767 : -32768;
	int numSamples = (samples > BUFSIZE) ? BUFSIZE : samples;
	for (int i=1; i<numSamples; i++) buffer[i] = nextSample;
	while (samples) {
		((CassetteDevice*)pluggable)->writeWave(buffer, numSamples);
		samples -= numSamples;
		numSamples = (samples > BUFSIZE) ? BUFSIZE : samples;
	}
}
