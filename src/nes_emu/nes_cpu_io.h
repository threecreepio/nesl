#pragma once

#define NES_CPU_READ_PPU( cpu, addr, time ) \
	STATIC_CAST(Nes_Core&,*cpu).cpu_read_ppu( addr, time )

#define NES_CPU_READ( cpu, addr, time ) \
	STATIC_CAST(Nes_Core&,*cpu).cpu_read( addr, time )

#define NES_CPU_WRITEX( cpu, addr, data, time ){\
	STATIC_CAST(Nes_Core&,*cpu).cpu_write( addr, data, time );\
}

#define NES_CPU_WRITE( cpu, addr, data, time ){\
	if ( addr < 0x800 ) cpu->low_mem [addr] = data;\
	else if ( addr == 0x2007 ) STATIC_CAST(Nes_Core&,*cpu).cpu_write_2007( data );\
	else STATIC_CAST(Nes_Core&,*cpu).cpu_write( addr, data, time );\
}

#define NES_CPU_WRITEDEBUG( cpu, addr, data, time ){\
	if ( memtracecb ) memtracecb( addr, data ); \
	if ( addr < 0x800 ) cpu->low_mem [addr] = data;\
	else if ( addr == 0x2007 ) STATIC_CAST(Nes_Core&,*cpu).cpu_write_2007( data );\
	else STATIC_CAST(Nes_Core&,*cpu).cpu_write( addr, data, time );\
}

#define NES_CPU_WRITELOWDEBUG( cpu, addr, data, time ) (memtracecb ? memtracecb(addr, data) : 0), (cpu->low_mem [addr] = data)

