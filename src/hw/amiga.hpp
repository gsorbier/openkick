// -*- c++ -*-
/**
   Headers for Amiga hardware.
   \file
*/

#ifndef HW_AMIGA_HPP
#define HW_AMIGA_HPP 1     //<! include guard

#include <types.hpp>

/**
   \brief Amiga hardware.
*/
namespace amiga {
    class CIA;
    class Custom;

    //! Physical base address of CIA A
    void * const CIAABase = reinterpret_cast<void * const>(0xbfe001);
    //! Physical base address of CIA B
    void * const CIABBase = reinterpret_cast<void * const>(0xbfd000);
    //! Physical base address of the custom chips
    void * const CustomBase = reinterpret_cast<void * const>(0xdff000);
}

/**
   \brief An 8520 CIA chip.

   An object of type CIA is a handle representing a single Amiga CIA chip.
   Use placement new to get a suitable object pointing at the logical
   (CPU-visible) address of the CIA chip in question.

*/
class amiga::CIA {
public:  volatile uint8_t pra; //!< Peripheral data register A
private: uint8_t _padCIA0[255];
public:  volatile uint8_t prb; //!< Peripheral data register B
private: uint8_t _padCIA1[255];
public:  volatile uint8_t ddra; //!< Data direction register A
private: uint8_t _padCIA2[255];
public:  volatile uint8_t ddrb; //!< Data direction register B
private: uint8_t _padCIA3[255];

public:  volatile uint8_t talo; //!< Timer A register, bits 0-7
private: uint8_t _padCIA4[255];
public:  volatile uint8_t tahi; //!< Timer A register, bits 8-15
private: uint8_t _padCIA5[255];
public:  volatile uint8_t tblo; //!< Timer B register, bits 0-7
private: uint8_t _padCIA6[255];
public:  volatile uint8_t tbhi; //!< Timer B register, bits 8-15
private: uint8_t _padCIA7[255];

public:  volatile uint8_t todlow; //!< Time of day counter, bits 0-7
private: uint8_t _padCIA8[255];
public:  volatile uint8_t todmid; //!< Time of day counter, bits 8-15
private: uint8_t _padCIA9[255];
public:  volatile uint8_t todhi; //!< Time of day counter, bits 16-23

private: uint8_t _padCIA10[511]; // as there is no register at offset 11 << 8.

public:  volatile uint8_t sdr; //!< Serial data register
private: uint8_t _padCIA11[255];
public:  volatile uint8_t icr; //!< Interrupt control register
private: uint8_t _padCIA12[255];
public:  volatile uint8_t cra; //!< Control register A
private: uint8_t _padCIA13[255];
public:  volatile uint8_t crb; //!< Control register B
};

/**
   \brief The Amiga custom chipset

   An object of type Custom is a handle representing the Amiga's custom
   chipset. Use placement new to get a suitable object pointing at the
   logical (CPU-visible) address of the custom chipset. Once you have
   this handle, you can use the accessor and mutator methods of the
   class which will directly access the custom chips on your behalf.
   Since the methods are inline, there is no efficiency loss compared to
   directly accessing the memory.

   \code
   amiga::Custom custom = new (amiga::CustomBase) amiga::Custom;

   custom->color(0, 0xfff);	// set a white background
   custom->color(1, 0xf0f);	// set COLOR01 to be magenta

   custom->dmacon(0x7fff);	// disable all DMA

   int data=custom->serdat()	// read data from serial port
   \endcode
*/
class amiga::Custom {
private:
    //! pointer to custom chip registers
    volatile union {
        int16_t reg16[0x100]; //!< registers as 16 bit values
        int32_t reg32[0x80]; //!< registers as 32 bit values
    };

public:

    //! Read a 16 bit value from the DMACON register
    uint16_t dmacon(void) volatile { return reg16[2/2]; }
    //! Read a 16 bit value from the VPOS register
    uint16_t vpos(void) volatile { return reg16[4/2]; }
    //! Read a 16 bit value from the VHPOS register
    uint16_t vhpos(void) volatile { return reg16[6/2]; }
    //! Read a 16 bit value from the JOY0DAT register
    uint16_t joy0dat(void) volatile { return reg16[10/2]; }
    //! Read a 16 bit value from the JOY1DAT register
    uint16_t joy1dat(void) volatile { return reg16[12/2]; }
    //! Read a 16 bit value from the CLXDAT register
    uint16_t clxdat(void) volatile { return reg16[14/2]; }
    //! Read a 16 bit value from the ADKCON register
    uint16_t adkcon(void) volatile { return reg16[16/2]; }
    //! Read a 16 bit value from the POT0DAT register
    uint16_t pot0dat(void) volatile { return reg16[18/2]; }
    //! Read a 16 bit value from the POT1DAT register
    uint16_t pot1dat(void) volatile { return reg16[20/2]; }
    //! Read a 16 bit value from the POTGO register
    uint16_t potgo(void) volatile { return reg16[22/2]; }
    //! Read a 16 bit value from the SERDAT register
    uint16_t serdat(void) volatile { return reg16[24/2]; }
    //! Read a 16 bit value from the DSKBYT register
    uint16_t dskbyt(void) volatile { return reg16[26/2]; }
    //! Read a 16 bit value from the INTENA register
    uint16_t intena(void) volatile { return reg16[28/2]; }
    //! Read a 16 bit value from the INTREQ register
    uint16_t intreq(void) volatile { return reg16[30/2]; }
    //! Write a 32 bit value to the DSKPT register pair
    template <typename ptr_t>void dskpt(volatile ptr_t *val) volatile {
        reg32[32/4]=reinterpret_cast<uint32_t>(val); 
    }
    //! Write a 16 bit value to the DSKLEN register
    void dsklen(uint16_t val) volatile { reg16[36/2]=val; }
    //! Write a 16 bit value to the VPOS register
    void vpos(uint16_t val) volatile { reg16[42/2]=val; }
    //! Write a 16 bit value to the VHPOS register
    void vhpos(uint16_t val) volatile { reg16[44/2]=val; }
    //! Write a 16 bit value to the COPCON register
    void copcon(uint16_t val) volatile { reg16[46/2]=val; }
    //! Write a 16 bit value to the SERDAT register
    void serdat(uint16_t val) volatile { reg16[48/2]=val; }
    //! Write a 16 bit value to the SERPER register
    void serper(uint16_t val) volatile { reg16[50/2]=val; }
    //! Write a 16 bit value to the PODGO register
    void podgo(uint16_t val) volatile { reg16[52/2]=val; }
    //! Write a 16 bit value to the JOYTEST register
    void joytest(uint16_t val) volatile { reg16[54/2]=val; }
    //! Write a 16 bit value to the BLTCON0 register
    void bltcon0(uint16_t val) volatile { reg16[64/2]=val; }
    //! Write a 16 bit value to the BLTCON1 register
    void bltcon1(uint16_t val) volatile { reg16[66/2]=val; }
    //! Write a 16 bit value to the BLTAFWM register
    void bltafwm(uint16_t val) volatile { reg16[68/2]=val; }
    //! Write a 16 bit value to the BLTALWM register
    void bltalwm(uint16_t val) volatile { reg16[70/2]=val; }
    //! Write a 32 bit value to the BLTCPT register pair
    template <typename ptr_t>void bltcpt(volatile ptr_t *val) volatile {
        reg32[72/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 32 bit value to the BLTBPT register pair
    template <typename ptr_t>void bltbpt(volatile ptr_t *val) volatile {
        reg32[76/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 32 bit value to the BLTAPT register pair
    template <typename ptr_t>void bltapt(volatile ptr_t *val) volatile {
        reg32[80/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 32 bit value to the BLTDPT register pair
    template <typename ptr_t>void bltdpt(volatile ptr_t *val) volatile {
        reg32[84/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 16 bit value to the BLTSIZE register
    void bltsize(uint16_t val) volatile { reg16[88/2]=val; }
    //! Write a 16 bit value to the BLTCMOD register
    void bltcmod(uint16_t val) volatile { reg16[96/2]=val; }
    //! Write a 16 bit value to the BLTBMOD register
    void bltbmod(uint16_t val) volatile { reg16[98/2]=val; }
    //! Write a 16 bit value to the BLTAMOD register
    void bltamod(uint16_t val) volatile { reg16[100/2]=val; }
    //! Write a 16 bit value to the BLTDMOD register
    void bltdmod(uint16_t val) volatile { reg16[102/2]=val; }
    //! Write a 16 bit value to the BLTCDAT register
    void bltcdat(uint16_t val) volatile { reg16[112/2]=val; }
    //! Write a 16 bit value to the BLTBDAT register
    void bltbdat(uint16_t val) volatile { reg16[114/2]=val; }
    //! Write a 16 bit value to the BLTADAT register
    void bltadat(uint16_t val) volatile { reg16[116/2]=val; }
    //! Write a 16 bit value to the BLTDDAT register
    void bltddat(uint16_t val) volatile { reg16[118/2]=val; }
    //! Write a 16 bit value to the DSKSYNC register
    void dsksync(uint16_t val) volatile { reg16[126/2]=val; }
    //! Write a 32 bit value to the COP1LC register pair
    template <typename ptr_t>void cop1lc(volatile ptr_t *val) volatile {
        reg32[128/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 32 bit value to the COP2LC register pair
    template <typename ptr_t>void cop2lc(volatile ptr_t *val) volatile {
        reg32[132/4]=reinterpret_cast<uint32_t>(val);
    }
    //! Strobe the COPJMP1 register
    void copjmp1(void) volatile { reg16[136/2]=0; }
    //! Strobe the COPJMP2 register
    void copjmp2(void) volatile { reg16[138/2]=0; }
    //! Write a 16 bit value to the COPINS register
    void copins(uint16_t val) volatile { reg16[140/2]=val; }
    //! Write a 16 bit value to the DIWSTRT register
    void diwstrt(uint16_t val) volatile { reg16[142/2]=val; }
    //! Write a 16 bit value to the DIWSTOP register
    void diwstop(uint16_t val) volatile { reg16[144/2]=val; }
    //! Write a 16 bit value to the DDFSTRT register
    void ddfstrt(uint16_t val) volatile { reg16[146/2]=val; }
    //! Write a 16 bit value to the DDFSTOP register
    void ddfstop(uint16_t val) volatile { reg16[148/2]=val; }
    //! Write a 16 bit value to the DMACON register
    void dmacon(uint16_t val) volatile { reg16[150/2]=val; }
    //! Write a 16 bit value to the CLXCON register
    void clxcon(uint16_t val) volatile { reg16[152/2]=val; }
    //! Write a 16 bit value to the INTENA register
    void intena(uint16_t val) volatile { reg16[154/2]=val; }
    //! Write a 16 bit value to the INTREQ register
    void intreq(uint16_t val) volatile { reg16[156/2]=val; }
    //! Write a 16 bit value to the ADKCON register
    void adkcon(uint16_t val) volatile { reg16[158/2]=val; }
    //! Write a 32 bit pointer to a AUDLC* register pair
    template <typename ptr_t>void audlc(size_t offset,
                                        volatile ptr_t *val) volatile {
        reg32[160/4+16*offset]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 16 bit value to a AUDLEN* register
    void audlen(size_t offset, uint16_t val) volatile {
        this->reg16[164/2+16*offset]=val;
    }
    //! Write a 16 bit value to a AUDPER* register
    void audper(size_t offset, uint16_t val) volatile {
        this->reg16[166/2+16*offset]=val;
    }
    //! Write a 16 bit value to a AUDVOL* register
    void audvol(size_t offset, uint16_t val) volatile {
        this->reg16[168/2+16*offset]=val;
    }
    //! Write a 16 bit value to a AUDDAT* register
    void auddat(size_t offset, uint16_t val) volatile {
        this->reg16[170/2+16*offset]=val;
    }
    //! Write a 32 bit pointer to a BPLPT* register pair
    template <typename ptr_t>void bplpt(size_t offset,
                                        volatile ptr_t *val) volatile {
        reg32[224/4+4*offset]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 16 bit value to the BPLCON0 register
    void bplcon0(uint16_t val) volatile {
        reg16[256/2]=val;
    }
    //! Write a 16 bit value to the BPLCON1 register
    void bplcon1(uint16_t val) volatile {
        reg16[258/2]=val;
    }
    //! Write a 16 bit value to the BPLCON2 register
    void bplcon2(uint16_t val) volatile {
        reg16[260/2]=val;
    }
    //! Write a 16 bit value to the BPL1MOD register
    void bpl1mod(uint16_t val) volatile {
        reg16[264/2]=val;
    }
    //! Write a 16 bit value to the BPL2MOD register
    void bpl2mod(uint16_t val) volatile {
        reg16[266/2]=val;
    }
    //! Write a 32 bit pointer to a SPRPT* register pair
    template <typename ptr_t>void sprpt(size_t offset,
                                        volatile ptr_t *val) volatile {
        reg32[288/4+4*offset]=reinterpret_cast<uint32_t>(val);
    }
    //! Write a 16 bit value to a SPRPOS* register
    void sprpos(size_t offset, uint16_t val) volatile {
        this->reg16[320/2+8*offset]=val;
    }
    //! Write a 16 bit value to a SPRCTL* register
    void sprctl(size_t offset, uint16_t val) volatile {
        this->reg16[322/2+8*offset]=val;
    }
    //! Write a 16 bit value to a SPRDATA* register
    void sprdata(size_t offset, uint16_t val) volatile {
        this->reg16[324/2+8*offset]=val;
    }
    //! Write a 16 bit value to a SPRDATB* register
    void sprdatb(size_t offset, uint16_t val) volatile {
        this->reg16[326/2+8*offset]=val;
    }
    //! Write a 16 bit value to a COLOR* register
    void color(size_t offset, uint16_t val) volatile {
        this->reg16[384/2+2*offset]=val;
    }
    //! Write a 16 bit value to the HTOTAL register
    void htotal(uint16_t val) volatile { reg16[448/2]=val; }
    //! Write a 16 bit value to the HSSTOP register
    void hsstop(uint16_t val) volatile { reg16[450/2]=val; }
    //! Write a 16 bit value to the HBSTRT register
    void hbstrt(uint16_t val) volatile { reg16[452/2]=val; }
    //! Write a 16 bit value to the HBSTOP register
    void hbstop(uint16_t val) volatile { reg16[454/2]=val; }
    //! Write a 16 bit value to the VTOTAL register
    void vtotal(uint16_t val) volatile { reg16[456/2]=val; }
    //! Write a 16 bit value to the VSSTOP register
    void vsstop(uint16_t val) volatile { reg16[458/2]=val; }
    //! Write a 16 bit value to the VBSTRT register
    void vbstrt(uint16_t val) volatile { reg16[460/2]=val; }
    //! Write a 16 bit value to the VBSTOP register
    void vbstop(uint16_t val) volatile { reg16[462/2]=val; }
    //! Write a 16 bit value to the SPRHSTRT register
    void sprhstrt(uint16_t val) volatile { reg16[464/2]=val; }
    //! Write a 16 bit value to the SPRHSTOP register
    void sprhstop(uint16_t val) volatile { reg16[466/2]=val; }
    //! Write a 16 bit value to the BPLHSTRT register
    void bplhstrt(uint16_t val) volatile { reg16[468/2]=val; }
    //! Write a 16 bit value to the BPLHSTOP register
    void bplhstop(uint16_t val) volatile { reg16[470/2]=val; }
    //! Write a 16 bit value to the HHPOS register
    void hhpos(uint16_t val) volatile { reg16[472/2]=val; }
    //! Write a 16 bit value to the HHPOSR register
    void hhposr(uint16_t val) volatile { reg16[474/2]=val; }
    //! Write a 16 bit value to the BEAMCON0 register
    void beamcon0(uint16_t val) volatile { reg16[476/2]=val; }
    //! Write a 16 bit value to the HSSTRT register
    void hsstrt(uint16_t val) volatile { reg16[478/2]=val; }
    //! Write a 16 bit value to the VSSTRT register
    void vsstrt(uint16_t val) volatile { reg16[480/2]=val; }
    //! Write a 16 bit value to the HCENTER register
    void hcenter(uint16_t val) volatile { reg16[482/2]=val; }
    //! Write a 16 bit value to the DIWHIGH register
    void diwhigh(uint16_t val) volatile { reg16[484/2]=val; }
    //! Write a 16 bit value to the FMODE register
    void fmode(uint16_t val) volatile { reg16[508/2]=val; }
    //! Write a 16 bit value to the NOOP register
    void noop(uint16_t val) volatile { reg16[510/2]=val; }

    /** Register offsets from the start of the custom chips, primarily used
        in copperlists */
    enum {

        // copper cannot access the below registers at all

        DMACONR = 0x002, VPOSR = 0x004, VHPOSR = 0x006,
        JOY0DAT = 0x00a, JOY1DAT = 0x00c, CLXDAT = 0x00e,
        ADKCONR = 0x010, POT0DAT = 0x012, POT1DAT = 0x014, POTGOR = 0x016,
        SERDATR = 0x018, DSKBYTR = 0x01a, INTENAR = 0x01c, INTREQR = 0x01e,
        DSKPTH = 0x020, DSKPTL = 0x022, DSKLEN = 0x024,
        VPOSW = 0x02a, VHPOSW = 0x02c, COPCON = 0x02e,
        SERDAT = 0x030, SERPER = 0x032, PODGO = 0x034, JOYTEST = 0x036,

        // copper can only access the below registers with the blitter danger flag set

        BLTCON0 = 0x040, BLTCON1 = 0x042, BLTAFWM = 0x044, BLTALWM = 0x046,
        BLTCPTH = 0x048, BLTCPTL = 0x04a, BLTBPTH = 0x04c, BLTBPTL = 0x04e,
        BLTAPTH = 0x050, BLTAPTL = 0x052, BLTDPTH = 0x054, BLTDPTL = 0x056,
        BLTSIZE = 0x058,
        BLTCMOD = 0x060, BLTBMOD = 0x062, BLTAMOD = 0x064, BLTDMOD = 0x066,
        BLTCDAT = 0x070, BLTBDAT = 0x072, BLTADAT = 0x074, BLTDDAT = 0x076,

        DSKSYNC = 0x07e,

        // copper can access all the below registers

        COP1LCH = 0x080, COP1LCL = 0x082, COP2LCH = 0x084, COP2LCL = 0x086,
        COPJMP1 = 0x088, COPJMP2 = 0x08a, COPINS = 0x08c, DIWSTRT = 0x08e,
        DIWSTOP = 0x090, DDFSTRT = 0x092, DDFSTOP = 0x094, DMACON = 0x096,
        CLXCON = 0x098, INTENA = 0x09a, INTREQ = 0x09c, ADKCON = 0x09e,

        AUD0LCH = 0x0a0, AUD0LCL = 0x0a2, AUD0LEN = 0x0a4, AUD0PER = 0x0a6,
        AUD0VOL = 0x0a8, AUD0DAT = 0x0aa,
        AUD1LCH = 0x0b0, AUD1LCL = 0x0b2, AUD1LEN = 0x0b4, AUD1PER = 0x0b6,
        AUD1VOL = 0x0b8, AUD1DAT = 0x0ba,
        AUD2LCH = 0x0c0, AUD2LCL = 0x0c2, AUD2LEN = 0x0c4, AUD2PER = 0x0c6,
        AUD2VOL = 0x0c8, AUD2DAT = 0x0ca,
        AUD3LCH = 0x0d0, AUD3LCL = 0x0d2, AUD3LEN = 0x0d4, AUD3PER = 0x0d6,
        AUD3VOL = 0x0d8, AUD3DAT = 0x0da,

        BPL0PTH = 0x0e0, BPL0PTL = 0x0e2, BPL1PTH = 0x0e4, BPL1PTL = 0x0e6,
        BPL2PTH = 0x0e8, BPL2PTL = 0x0ea, BPL3PTH = 0x0ec, BPL3PTL = 0x0ee,
        BPL4PTH = 0x0f0, BPL4PTL = 0x0f2, BPL5PTH = 0x0f4, BPL5PTL = 0x0f6,
        BPLCON0 = 0x100, BPLCON1 = 0x102, BPLCON2 = 0x104,
        BPL1MOD = 0x108, BPL2MOD = 0x10a,

        SPR0PTH = 0x120, SPR0PTL = 0x122, SPR1PTH = 0x124, SPR1PTL = 0x126,
        SPR2PTH = 0x128, SPR2PTL = 0x12a, SPR3PTH = 0x12c, SPR3PTL = 0x12e,
        SPR4PTH = 0x130, SPR4PTL = 0x132, SPR5PTH = 0x134, SPR5PTL = 0x136,
        SPR6PTH = 0x138, SPR6PTL = 0x13a, SPR7PTH = 0x13c, SPR7PTL = 0x13e,
        SPR0POS = 0x140, SPR0CTL = 0x142, SPR0DATA = 0x144, SPR0DATB = 0x146,
        SPR1POS = 0x148, SPR1CTL = 0x14a, SPR1DATA = 0x14c, SPR1DATB = 0x14e,
        SPR2POS = 0x150, SPR2CTL = 0x152, SPR2DATA = 0x154, SPR2DATB = 0x156,
        SPR3POS = 0x158, SPR3CTL = 0x15a, SPR3DATA = 0x15c, SPR3DATB = 0x15e,
        SPR4POS = 0x160, SPR4CTL = 0x162, SPR4DATA = 0x164, SPR4DATB = 0x166,
        SPR5POS = 0x168, SPR5CTL = 0x16a, SPR5DATA = 0x16c, SPR5DATB = 0x16e,
        SPR6POS = 0x170, SPR6CTL = 0x172, SPR6DATA = 0x174, SPR6DATB = 0x176,
        SPR7POS = 0x178, SPR7CTL = 0x17a, SPR7DATA = 0x17c, SPR7DATB = 0x17e,

        COLOR00 = 0x180, COLOR01 = 0x182, COLOR02 = 0x184, COLOR03 = 0x186,
        COLOR04 = 0x188, COLOR05 = 0x18a, COLOR06 = 0x18c, COLOR07 = 0x18e,
        COLOR08 = 0x190, COLOR09 = 0x192, COLOR10 = 0x194, COLOR11 = 0x196,
        COLOR12 = 0x198, COLOR13 = 0x19a, COLOR14 = 0x19c, COLOR15 = 0x19e,
        COLOR16 = 0x1a0, COLOR17 = 0x1a2, COLOR18 = 0x1a4, COLOR19 = 0x1a6,
        COLOR20 = 0x1a8, COLOR21 = 0x1aa, COLOR22 = 0x1ac, COLOR23 = 0x1ae,
        COLOR24 = 0x1b0, COLOR25 = 0x1b2, COLOR26 = 0x1b4, COLOR27 = 0x1b6,
        COLOR28 = 0x1b8, COLOR29 = 0x1ba, COLOR30 = 0x1bc, COLOR31 = 0x1be,

        // not available on all chipsets

        HTOTAL = 0x1c0, HSSTOP = 0x1c2, HBSTRT = 0x1c4, HBSTOP = 0x1c6,
        VTOTAL = 0x1c8, VSSTOP = 0x1ca, VBSTRT = 0x1cc, VBSTOP = 0x1ce,
        SPRHSTRT = 0x1d0, SPRHSTOP = 0x1d2, BPLHSTRT = 0x1d4, BPLHSTOP = 0x1d6,
        HHPOSW = 0x1d8, HHPOSR = 0x1da, BEAMCON0 = 0x1dc, HSSTRT = 0x1de,
        VSSTRT = 0x1e0, HCENTER = 0x1e2, DIWHIGH = 0x1e4, FMODE = 0x1fc,
        NOOP = 0x1fe
    }; // enum

};

#endif
