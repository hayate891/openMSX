// $Id$

#ifndef __MC6850_HH__
#define __MC6850_HH__

#include "MSXIODevice.hh"

namespace openmsx {

class MC6850 : public MSXIODevice
{
public:
	MC6850(Config* config, const EmuTime& time);
	virtual ~MC6850(); 
	
	virtual void reset(const EmuTime& time);
	virtual byte readIO(byte port, const EmuTime& time);
	virtual void writeIO(byte port, byte value, const EmuTime& time);
};

} // namespace openmsx
#endif
