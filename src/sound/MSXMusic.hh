// $Id$

#ifndef __MSXMUSIC_HH__
#define __MSXMUSIC_HH__

#include "MSXIODevice.hh"
#include "MSXMemDevice.hh"
#include "Rom.hh"

namespace openmsx {

class YM2413Core;

class MSXMusic : public MSXIODevice, public MSXMemDevice
{
public:
	MSXMusic(Config* config, const EmuTime& time);
	virtual ~MSXMusic();
	
	virtual void reset(const EmuTime& time);
	virtual void writeIO(byte port, byte value, const EmuTime& time);
	virtual byte readMem(word address, const EmuTime& time);
	virtual const byte* getReadCacheLine(word start) const;

protected:
	void writeRegisterPort(byte value, const EmuTime& time);
	void writeDataPort(byte value, const EmuTime& time);

	Rom rom;
	YM2413Core* ym2413;

private:
	int registerLatch;
};

} // namespace openmsx

#endif
