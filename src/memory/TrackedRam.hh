#ifndef TRACKED_RAM_HH
#define TRACKED_RAM_HH

#include "Ram.hh"

namespace openmsx {

// Ram with dirty tracking
class TrackedRam
{
public:
	// Most methods simply delegate to the internal 'ram' object.
	TrackedRam(const DeviceConfig& config, const std::string& name,
	           const std::string& description, unsigned size)
		: ram(config, name, description, size) {}

	TrackedRam(const DeviceConfig& config, unsigned size)
		: ram(config, size) {}

	unsigned getSize() const {
		return ram.getSize();
	}

	const std::string& getName() const {
		return ram.getName();
	}

	// Allow read via an explicit read() method or via backdoor access.
	byte read(unsigned addr) const {
		return ram[addr];
	}

	const byte& operator[](unsigned addr) const {
		return ram[addr];
	}

	// Only allow write/clear via an explicit method.
	void write(unsigned addr, byte value) {
		writeSinceLastReverseSnapshot = true;
		ram[addr] = value;
	}

	void clear(byte c = 0xff) {
		writeSinceLastReverseSnapshot = true;
		ram.clear(c);
	}

	// Some write operations are more efficient in bulk. For those this
	// method can be used. It will mark the ram as dirty on each
	// invocation, so the resulting pointer (although the same each time)
	// should not be reused for multiple (distinct) bulk write operations.
	byte* getWriteBackdoor() {
		writeSinceLastReverseSnapshot = true;
		return &ram[0];
	}

	template<typename Archive>
	void serialize(Archive& ar, unsigned version);

private:
	Ram ram;
	bool writeSinceLastReverseSnapshot = true;
};

} // namespace openmsx

#endif
