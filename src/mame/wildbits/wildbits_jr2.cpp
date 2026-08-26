// license:BSD-3-Clause
// copyright-holders:Richard Lucente, Antigravity
/***************************************************************************

    Wildbits Jr2 (FNX6809 Core) Emulation

    The Wildbits Jr2 (formerly Foenix F256 Jr2) is an FPGA-based retro
    system powered by an Artix-7 FPGA running the FNX6809 soft core,
    TinyVicky II graphics, 512KB SRAM, 512KB Flash ROM, and integrated
    peripherals.

****************************************************************************/

#include "emu.h"

#include "cpu/m6809/m6809.h"
#include "machine/spi_sdcard.h"
#include "screen.h"
#include <queue>

namespace {

// Standard 8x8 ASCII Font Bitmap for Characters 32-127 (8 bytes per character)
static const uint8_t s_default_font_8x8[96 * 8] = {
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, //   (32)
	0x18,0x3c,0x3c,0x18,0x18,0x00,0x18,0x00, // ! (33)
	0x66,0x66,0x24,0x00,0x00,0x00,0x00,0x00, // " (34)
	0x6c,0x6c,0xfe,0x6c,0xfe,0x6c,0x6c,0x00, // # (35)
	0x18,0x3e,0x60,0x3c,0x06,0x7c,0x18,0x00, // $ (36)
	0x00,0x66,0xa6,0xd4,0x2b,0x65,0x66,0x00, // % (37)
	0x38,0x6c,0x38,0x76,0xdc,0xcc,0x76,0x00, // & (38)
	0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00, // ' (39)
	0x0c,0x18,0x30,0x30,0x30,0x18,0x0c,0x00, // ( (40)
	0x30,0x18,0x0c,0x0c,0x0c,0x18,0x30,0x00, // ) (41)
	0x00,0x66,0x3c,0xff,0x3c,0x66,0x00,0x00, // * (42)
	0x00,0x18,0x18,0x7e,0x18,0x18,0x00,0x00, // + (43)
	0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30, // , (44)
	0x00,0x00,0x00,0x7e,0x00,0x00,0x00,0x00, // - (45)
	0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00, // . (46)
	0x06,0x0c,0x18,0x30,0x60,0xc0,0x80,0x00, // / (47)
	0x7c,0xc6,0xce,0xd6,0xe6,0xc6,0x7c,0x00, // 0 (48)
	0x18,0x38,0x18,0x18,0x18,0x18,0x7e,0x00, // 1 (49)
	0x7c,0xc6,0x06,0x1c,0x30,0x66,0xfe,0x00, // 2 (50)
	0x7c,0xc6,0x06,0x3c,0x06,0xc6,0x7c,0x00, // 3 (51)
	0x1c,0x3c,0x6c,0xcc,0xfe,0x0c,0x1e,0x00, // 4 (52)
	0xfe,0xc0,0xfc,0x06,0x06,0xc6,0x7c,0x00, // 5 (53)
	0x38,0x60,0xc0,0xfc,0xc6,0xc6,0x7c,0x00, // 6 (54)
	0xfe,0xc6,0x0c,0x18,0x30,0x30,0x30,0x00, // 7 (55)
	0x7c,0xc6,0xc6,0x7c,0xc6,0xc6,0x7c,0x00, // 8 (56)
	0x7c,0xc6,0xc6,0x7e,0x06,0x0c,0x78,0x00, // 9 (57)
	0x00,0x18,0x18,0x00,0x18,0x18,0x00,0x00, // : (58)
	0x00,0x18,0x18,0x00,0x18,0x18,0x30,0x00, // ; (59)
	0x06,0x0c,0x18,0x30,0x18,0x0c,0x06,0x00, // < (60)
	0x00,0x00,0x7e,0x00,0x7e,0x00,0x00,0x00, // = (61)
	0x60,0x30,0x18,0x0c,0x18,0x30,0x60,0x00, // > (62)
	0x7c,0xc6,0x0c,0x18,0x18,0x00,0x18,0x00, // ? (63)
	0x7c,0xc6,0xde,0xde,0xdc,0xc0,0x7c,0x00, // @ (64)
	0x38,0x6c,0xc6,0xfe,0xc6,0xc6,0xc6,0x00, // A (65)
	0xfc,0x66,0x66,0x7c,0x66,0x66,0xfc,0x00, // B (66)
	0x3c,0x66,0xc0,0xc0,0xc0,0x66,0x3c,0x00, // C (67)
	0xf8,0x6c,0x66,0x66,0x66,0x6c,0xf8,0x00, // D (68)
	0xfe,0x62,0x68,0x78,0x68,0x62,0xfe,0x00, // E (69)
	0xfe,0x62,0x68,0x78,0x68,0x60,0xf0,0x00, // F (70)
	0x3c,0x66,0xc0,0xc0,0xce,0x66,0x3e,0x00, // G (71)
	0xc6,0xc6,0xc6,0xfe,0xc6,0xc6,0xc6,0x00, // H (72)
	0x7e,0x18,0x18,0x18,0x18,0x18,0x7e,0x00, // I (73)
	0x1e,0x0c,0x0c,0x0c,0xcc,0xcc,0x78,0x00, // J (74)
	0xe6,0x66,0x6c,0x78,0x6c,0x66,0xe6,0x00, // K (75)
	0xf0,0x60,0x60,0x60,0x62,0x66,0xfe,0x00, // L (76)
	0xc6,0xee,0xfe,0xfe,0xd6,0xc6,0xc6,0x00, // M (77)
	0xc6,0xe6,0xf6,0xde,0xce,0xc6,0xc6,0x00, // N (78)
	0x7c,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00, // O (79)
	0xfc,0x66,0x66,0x7c,0x60,0x60,0xf0,0x00, // P (80)
	0x7c,0xc6,0xc6,0xc6,0xd6,0x7c,0x0e,0x00, // Q (81)
	0xfc,0x66,0x66,0x7c,0x6c,0x66,0xe6,0x00, // R (82)
	0x7c,0xc6,0x60,0x3c,0x06,0xc6,0x7c,0x00, // S (83)
	0x7e,0x5a,0x18,0x18,0x18,0x18,0x3c,0x00, // T (84)
	0xc6,0xc6,0xc6,0xc6,0xc6,0xc6,0x7c,0x00, // U (85)
	0xc6,0xc6,0xc6,0xc6,0xc6,0x6c,0x38,0x00, // V (86)
	0xc6,0xc6,0xd6,0xfe,0xfe,0xee,0xc6,0x00, // W (87)
	0xc6,0xc6,0x6c,0x38,0x6c,0xc6,0xc6,0x00, // X (88)
	0x66,0x66,0x66,0x3c,0x18,0x18,0x3c,0x00, // Y (89)
	0xfe,0xc6,0x8c,0x18,0x32,0x66,0xfe,0x00, // Z (90)
	0x3c,0x30,0x30,0x30,0x30,0x30,0x3c,0x00, // [ (91)
	0xc0,0x60,0x30,0x18,0x0c,0x06,0x02,0x00, // \ (92)
	0x3c,0x0c,0x0c,0x0c,0x0c,0x0c,0x3c,0x00, // ] (93)
	0x10,0x38,0x6c,0xc6,0x00,0x00,0x00,0x00, // ^ (94)
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff, // _ (95)
	0x30,0x18,0x0c,0x00,0x00,0x00,0x00,0x00, // ` (96)
	0x00,0x00,0x78,0x0c,0x7c,0xcc,0x76,0x00, // a (97)
	0xe0,0x60,0x7c,0x66,0x66,0x66,0xdc,0x00, // b (98)
	0x00,0x00,0x7c,0xc6,0xc0,0xc6,0x7c,0x00, // c (99)
	0x1c,0x0c,0x7c,0xcc,0xcc,0xcc,0x76,0x00, // d (100)
	0x00,0x00,0x7c,0xc6,0xfe,0xc0,0x7c,0x00, // e (101)
	0x1c,0x36,0x30,0x78,0x30,0x30,0x78,0x00, // f (102)
	0x00,0x00,0x76,0xcc,0xcc,0x7c,0x0c,0xf8, // g (103)
	0xe0,0x60,0x6c,0x76,0x66,0x66,0xe6,0x00, // h (104)
	0x18,0x00,0x38,0x18,0x18,0x18,0x3c,0x00, // i (105)
	0x06,0x00,0x06,0x06,0x06,0x66,0x66,0x3c, // j (106)
	0xe0,0x60,0x66,0x6c,0x78,0x6c,0xe6,0x00, // k (107)
	0x38,0x18,0x18,0x18,0x18,0x18,0x3c,0x00, // l (108)
	0x00,0x00,0xec,0xfe,0xd6,0xd6,0xd6,0x00, // m (109)
	0x00,0x00,0xdc,0x66,0x66,0x66,0x66,0x00, // n (110)
	0x00,0x00,0x7c,0xc6,0xc6,0xc6,0x7c,0x00, // o (111)
	0x00,0x00,0xdc,0x66,0x66,0x7c,0x60,0xf0, // p (112)
	0x00,0x00,0x76,0xcc,0xcc,0x7c,0x0c,0x1e, // q (113)
	0x00,0x00,0xdc,0x76,0x60,0x60,0xf0,0x00, // r (114)
	0x00,0x00,0x7c,0xc0,0x7c,0x06,0xfc,0x00, // s (115)
	0x30,0x30,0x7c,0x30,0x30,0x36,0x1c,0x00, // t (116)
	0x00,0x00,0xcc,0xcc,0xcc,0xcc,0x76,0x00, // u (117)
	0x00,0x00,0xc6,0xc6,0xc6,0x6c,0x38,0x00, // v (118)
	0x00,0x00,0xc6,0xd6,0xd6,0xfe,0x6c,0x00, // w (119)
	0x00,0x00,0xc6,0x6c,0x38,0x6c,0xc6,0x00, // x (120)
	0x00,0x00,0xc6,0xc6,0xce,0x76,0x06,0xfc, // y (121)
	0x00,0x00,0xfe,0x8c,0x18,0x32,0xfe,0x00, // z (122)
	0x0e,0x18,0x18,0x70,0x18,0x18,0x0e,0x00, // { (123)
	0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x00, // | (124)
	0x70,0x18,0x18,0x0e,0x18,0x18,0x70,0x00, // } (125)
	0x76,0xdc,0x00,0x00,0x00,0x00,0x00,0x00  // ~ (126)
};

// Standard 16-Color Palette (RGB)
static const rgb_t s_default_palette[16] = {
	rgb_t(0x00, 0x00, 0x00), // 0: Black
	rgb_t(0x00, 0x00, 0xAA), // 1: Blue
	rgb_t(0x00, 0xAA, 0x00), // 2: Green
	rgb_t(0x00, 0xAA, 0xAA), // 3: Cyan
	rgb_t(0xAA, 0x00, 0x00), // 4: Red
	rgb_t(0xAA, 0x00, 0xAA), // 5: Magenta
	rgb_t(0xAA, 0x55, 0x00), // 6: Brown
	rgb_t(0xAA, 0xAA, 0xAA), // 7: Light Gray
	rgb_t(0x55, 0x55, 0x55), // 8: Dark Gray
	rgb_t(0x55, 0x55, 0xFF), // 9: Light Blue
	rgb_t(0x55, 0xFF, 0x55), // 10: Light Green
	rgb_t(0x55, 0xFF, 0xFF), // 11: Light Cyan
	rgb_t(0xFF, 0x55, 0x55), // 12: Light Red
	rgb_t(0xFF, 0x55, 0xFF), // 13: Light Magenta
	rgb_t(0xFF, 0xFF, 0x55), // 14: Yellow
	rgb_t(0xFF, 0xFF, 0xFF)  // 15: White
};

class wildbits_jr2_state : public driver_device
{
public:
	wildbits_jr2_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_sdcard(*this, "sdcard")
		, m_screen(*this, "screen")
		, m_flash(*this, "flash")
		, m_bank(*this, "bank%u", 0U)
		, m_io_key(*this, "KEY%u", 0U)
		, m_frame_count(0)
	{ }

	void wbjr2(machine_config &config) ATTR_COLD;

protected:
	virtual void machine_start() override ATTR_COLD;
	virtual void machine_reset() override ATTR_COLD;
	virtual void device_stop() override ATTR_COLD;

private:
	void wbjr2_mem(address_map &map) ATTR_COLD;
	uint32_t screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect);
	void poll_keyboard();

	// MMU handlers
	uint8_t mmu_mem_ctrl_r();
	void mmu_mem_ctrl_w(uint8_t data);
	uint8_t mmu_io_ctrl_r();
	void mmu_io_ctrl_w(uint8_t data);
	uint8_t mmu_slot_r(offs_t offset);
	void mmu_slot_w(offs_t offset, uint8_t data);

	// System Control handlers
	uint8_t sys0_r();
	void sys0_w(uint8_t data);
	uint8_t sys1_r();
	void sys1_w(uint8_t data);
	void rst0_w(uint8_t data);
	void rst1_w(uint8_t data);
	uint8_t mid_r();

	// Real-Time Clock ($FE10 - $FE1F)
	uint8_t rtc_r(offs_t offset);
	void rtc_w(offs_t offset, uint8_t data);

	// Interrupt Controller ($FE20 - $FE2F)
	uint8_t intc_r(offs_t offset);
	void intc_w(offs_t offset, uint8_t data);
	void check_irqs();
	void set_irq(int group, uint8_t mask);

	// System Timers ($FE30 - $FE3F)
	uint8_t timer_r(offs_t offset);
	void timer_w(offs_t offset, uint8_t data);
	TIMER_CALLBACK_MEMBER(timer0_tick);
	TIMER_CALLBACK_MEMBER(timer1_tick);

	// PS/2 Keyboard & Mouse ($FE50 - $FE54)
	uint8_t ps2_r(offs_t offset);
	void ps2_w(offs_t offset, uint8_t data);
	void queue_kbd_scancode(uint8_t scancode);

	// 16550 UART ($FE60 - $FE67)
	uint8_t uart_r(offs_t offset);
	void uart_w(offs_t offset, uint8_t data);
	void poll_uart_socket();

	// Audio CODEC ($FE70 - $FE72)
	uint8_t codec_r(offs_t offset);
	void codec_w(offs_t offset, uint8_t data);

	// SPI SD Card ($FE90 - $FE91)
	uint8_t sdc_stat_r();
	void sdc_stat_w(uint8_t data);
	uint8_t sdc_data_r();
	void sdc_data_w(uint8_t data);
	void sdcard_miso_w(int state);

	// WizFi360 WiFi / SPI ($FF20 - $FF2F)
	uint8_t wizfi_r(offs_t offset);
	void wizfi_w(offs_t offset, uint8_t data);
	void push_wizfi_response(const std::string &resp);
	void process_wizfi_cmd(const std::string &cmd_raw);
	void handle_cipstart(const std::string &cmd);
	void handle_cipsend(const std::string &cmd);
	void poll_wizfi_socket();
	void reset_wizfi();

	// TinyVicky Video handlers ($FFC0 - $FFDF)
	uint8_t vky_r(offs_t offset);
	void vky_w(offs_t offset, uint8_t data);
	void vblank_w(int state);

	void update_banks();
	uint8_t *get_physical_block_ptr(uint8_t block_num);

	required_device<cpu_device> m_maincpu;
	required_device<spi_sdcard_device> m_sdcard;
	required_device<screen_device> m_screen;
	required_region_ptr<uint8_t> m_flash;
	memory_bank_array_creator<8> m_bank;

	// Memory structures
	std::unique_ptr<uint8_t[]> m_ram;        // 512KB SRAM
	std::unique_ptr<uint8_t[]> m_vram_c0;     // Block $C0: VICKY control & gamma
	std::unique_ptr<uint8_t[]> m_vram_c1;     // Block $C1: Font RAM & Palettes
	std::unique_ptr<uint8_t[]> m_vram_c2;     // Block $C2: Text Character Matrix
	std::unique_ptr<uint8_t[]> m_vram_c3;     // Block $C3: Text Color Matrix
	std::unique_ptr<uint8_t[]> m_vram_c4;     // Block $C4: Audio registers
	uint8_t m_constant_ram[256];
	uint8_t m_vector_ram[16];

	// MMU state
	uint8_t m_mlut[4][8];
	uint8_t m_mmu_mem_ctrl;
	uint8_t m_mmu_io_ctrl;

	// System registers
	uint8_t m_sys0;
	uint8_t m_sys1;
	uint8_t m_rst0;
	uint8_t m_rst1;

	// Interrupt Controller
	uint8_t m_int_pending[4];
	uint8_t m_int_pol[4];
	uint8_t m_int_edge[4];
	uint8_t m_int_mask[4];

	// Timers
	emu_timer *m_timer0;
	emu_timer *m_timer1;
	uint8_t m_t0_ctr;
	uint8_t m_t0_stat;
	uint32_t m_t0_val;
	uint32_t m_t0_cmp;
	uint8_t m_t0_cmp_ctr;

	uint8_t m_t1_ctr;
	uint8_t m_t1_stat;
	uint32_t m_t1_val;
	uint32_t m_t1_cmp;
	uint8_t m_t1_cmp_ctr;

	// PS/2 Controller
	uint8_t m_ps2_ctrl;
	uint8_t m_ps2_out;
	std::queue<uint8_t> m_kbd_fifo;
	std::queue<uint8_t> m_mouse_fifo;

	// 16550 UART state
	uint8_t m_uart_dll;
	uint8_t m_uart_dlh;
	uint8_t m_uart_ier;
	uint8_t m_uart_fcr;
	uint8_t m_uart_lcr;
	uint8_t m_uart_mcr;
	uint8_t m_uart_scr;
	std::queue<uint8_t> m_uart_rx_fifo;
	osd_file::ptr m_uart_socket;

	// RTC state
	uint8_t m_rtc_ctrl;

	// Audio CODEC state
	uint8_t m_codec_lo;
	uint8_t m_codec_hi;

	// SPI SD Card state
	uint8_t m_sdc_stat;
	uint8_t m_sdc_data_in;
	uint8_t m_sdc_data_out;
	int m_sdcard_miso;

	// WizFi360 State
	std::queue<uint8_t> m_wizfi_rx_fifo;
	std::string m_wizfi_tx_buf;
	uint8_t m_wizfi_ctrl;
	bool m_wizfi_transparent;
	int m_wizfi_plus_count;
	int m_wizfi_cipsend_remaining;
	int m_wizfi_cipsend_total;
	bool m_wizfi_cipmux;
	bool m_wizfi_cipmode;
	bool m_wizfi_echo;
	int m_wizfi_cwmode;
	bool m_wizfi_wifi_connected;
	std::string m_wizfi_ssid;
	int m_wizfi_link_id;
	std::string m_wizfi_remote_host;
	int m_wizfi_remote_port;
	osd_file::ptr m_wizfi_socket;

	// TinyVicky Master Registers
	uint8_t m_vky_mstr_ctrl_0;
	uint8_t m_vky_mstr_ctrl_1;
	uint8_t m_vky_layer_ctrl_0;
	uint8_t m_vky_layer_ctrl_1;
	uint8_t m_vky_brdr_ctrl;
	uint8_t m_vky_brdr_b;
	uint8_t m_vky_brdr_g;
	uint8_t m_vky_brdr_r;
	uint8_t m_vky_brdr_w;
	uint8_t m_vky_brdr_h;
	uint8_t m_vky_bg_b;
	uint8_t m_vky_bg_g;
	uint8_t m_vky_bg_r;
	uint8_t m_vky_crsr_ctrl;
	uint8_t m_vky_crsr_char;
	uint8_t m_vky_crsr_color;
	uint16_t m_vky_crsr_x;
	uint16_t m_vky_crsr_y;

	// Keyboard Input Ports
	required_ioport_array<4> m_io_key;
	uint16_t m_key_state[4];
	uint32_t m_frame_count;
};

// Machine ID for F256 Jr2 (6809 core) is 0x1A
constexpr uint8_t WBJR2_MACHINE_ID = 0x1a;

uint8_t *wildbits_jr2_state::get_physical_block_ptr(uint8_t block_num)
{
	// 21-bit physical space: 8KB per block
	// Blocks 0x00 - 0x3F (0x000000 - 0x07FFFF): 512KB SRAM
	// Blocks 0x40 - 0x7F (0x080000 - 0x0FFFFF): 512KB Flash ROM
	// Blocks 0xC0 - 0xC4: Dedicated Video and Audio Block buffers
	if (block_num < 0x40)
	{
		return &m_ram[(block_num & 0x3f) * 0x2000];
	}
	else if (block_num < 0x80)
	{
		return &m_flash[(block_num - 0x40) * 0x2000];
	}
	else if (block_num == 0xc0)
	{
		return m_vram_c0.get();
	}
	else if (block_num == 0xc1)
	{
		return m_vram_c1.get();
	}
	else if (block_num == 0xc2)
	{
		return m_vram_c2.get();
	}
	else if (block_num == 0xc3)
	{
		return m_vram_c3.get();
	}
	else if (block_num == 0xc4)
	{
		return m_vram_c4.get();
	}
	else
	{
		return &m_ram[(block_num & 0x3f) * 0x2000];
	}
}

void wildbits_jr2_state::update_banks()
{
	uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
	for (int slot = 0; slot < 8; slot++)
	{
		uint8_t block = m_mlut[active_lut][slot];
		m_bank[slot]->set_base(get_physical_block_ptr(block));
	}
}

uint8_t wildbits_jr2_state::mmu_mem_ctrl_r()
{
	return m_mmu_mem_ctrl;
}

void wildbits_jr2_state::mmu_mem_ctrl_w(uint8_t data)
{
	m_mmu_mem_ctrl = data;
	update_banks();
}

uint8_t wildbits_jr2_state::mmu_io_ctrl_r()
{
	return m_mmu_io_ctrl;
}

void wildbits_jr2_state::mmu_io_ctrl_w(uint8_t data)
{
	m_mmu_io_ctrl = data;
}

uint8_t wildbits_jr2_state::mmu_slot_r(offs_t offset)
{
	uint8_t lut = ((m_mmu_mem_ctrl & 0x80) || (m_mmu_mem_ctrl & 0x30)) ? ((m_mmu_mem_ctrl >> 4) & 0x03) : (m_mmu_mem_ctrl & 0x03);
	return m_mlut[lut][offset & 0x07];
}

void wildbits_jr2_state::mmu_slot_w(offs_t offset, uint8_t data)
{
	uint8_t lut = ((m_mmu_mem_ctrl & 0x80) || (m_mmu_mem_ctrl & 0x30)) ? ((m_mmu_mem_ctrl >> 4) & 0x03) : (m_mmu_mem_ctrl & 0x03);
	m_mlut[lut][offset & 0x07] = data;
	if (lut == (m_mmu_mem_ctrl & 0x03))
	{
		update_banks();
	}
}

uint8_t wildbits_jr2_state::sys0_r()
{
	uint8_t val = m_sys0 & ~0xc0;
	if (!m_sdcard->get_card_present())
	{
		val |= 0x40; // SYS_SD_CD: 1 = no card, 0 = card inserted
	}
	return val;
}

void wildbits_jr2_state::sys0_w(uint8_t data)
{
	m_sys0 = data;
	if ((data & 0x80) && (m_rst0 == 0xde) && (m_rst1 == 0xad))
	{
		printf("DEBUG: Software RESET via SYS0! PC=0x%04X\n", m_maincpu->pc());
		machine_reset();
	}
}

uint8_t wildbits_jr2_state::sys1_r()
{
	return m_sys1;
}

void wildbits_jr2_state::sys1_w(uint8_t data)
{
	m_sys1 = data;
}

void wildbits_jr2_state::rst0_w(uint8_t data)
{
	m_rst0 = data;
}

void wildbits_jr2_state::rst1_w(uint8_t data)
{
	m_rst1 = data;
}

uint8_t wildbits_jr2_state::mid_r()
{
	return WBJR2_MACHINE_ID;
}

// Real-Time Clock ($FE10 - $FE1F: bq4802)
static inline uint8_t to_bcd(uint8_t val)
{
	return ((val / 10) << 4) | (val % 10);
}

uint8_t wildbits_jr2_state::rtc_r(offs_t offset)
{
	system_time systime;
	machine().current_datetime(systime);
	switch (offset)
	{
	case 0x00: return to_bcd(systime.local_time.second);
	case 0x02: return to_bcd(systime.local_time.minute);
	case 0x04: return to_bcd(systime.local_time.hour);
	case 0x06: return to_bcd(systime.local_time.mday);
	case 0x08: return to_bcd(systime.local_time.month + 1);
	case 0x09: return to_bcd(systime.local_time.year % 100);
	case 0x0e: return m_rtc_ctrl;
	default: return 0;
	}
}

void wildbits_jr2_state::rtc_w(offs_t offset, uint8_t data)
{
	if (offset == 0x0e)
		m_rtc_ctrl = data;
}

// Audio CODEC ($FE70 - $FE72: WM8731)
uint8_t wildbits_jr2_state::codec_r(offs_t offset)
{
	if (offset == 2)
		return 0x00; // Bit 0 = 0 (Ready / idle)
	return (offset == 0) ? m_codec_lo : m_codec_hi;
}

void wildbits_jr2_state::codec_w(offs_t offset, uint8_t data)
{
	if (offset == 0) m_codec_lo = data;
	else if (offset == 1) m_codec_hi = data;
}

// Interrupt Controller ($FE20 - $FE2F)
void wildbits_jr2_state::check_irqs()
{
	bool assert_irq = false;
	for (int g = 0; g < 4; g++)
	{
		if ((m_int_pending[g] & ~m_int_mask[g]) != 0)
		{
			assert_irq = true;
			break;
		}
	}
	m_maincpu->set_input_line(M6809_IRQ_LINE, assert_irq ? ASSERT_LINE : CLEAR_LINE);
}

void wildbits_jr2_state::set_irq(int group, uint8_t mask)
{
	m_int_pending[group] |= mask;
	check_irqs();
}

uint8_t wildbits_jr2_state::intc_r(offs_t offset)
{
	switch (offset >> 2)
	{
	case 0: return m_int_pending[offset & 3];
	case 1: return m_int_pol[offset & 3];
	case 2: return m_int_edge[offset & 3];
	case 3: return m_int_mask[offset & 3];
	default: return 0;
	}
}

void wildbits_jr2_state::intc_w(offs_t offset, uint8_t data)
{
	switch (offset >> 2)
	{
	case 0: // Write 1 to clear pending bit
		m_int_pending[offset & 3] &= ~data;
		check_irqs();
		break;
	case 1:
		m_int_pol[offset & 3] = data;
		break;
	case 2:
		m_int_edge[offset & 3] = data;
		break;
	case 3: // Mask register (0=unmasked, 1=masked)
		m_int_mask[offset & 3] = data;
		check_irqs();
		break;
	}
}

// System Timers ($FE30 - $FE3F)
TIMER_CALLBACK_MEMBER(wildbits_jr2_state::timer0_tick)
{
	m_t0_stat |= 0x01; // Compare match status
	set_irq(0, 0x10);  // INT_TIMER_0 (Bit 4 of Group 0)

	poll_wizfi_socket();

	if (m_t0_cmp > 0 && (m_t0_ctr & 0x01))
	{
		attotime period = attotime::from_hz(25'175'000) * m_t0_cmp;
		if (period < attotime::from_hz(1000))
			period = attotime::from_hz(1000);
		m_timer0->adjust(period);
	}
}

TIMER_CALLBACK_MEMBER(wildbits_jr2_state::timer1_tick)
{
	m_t1_stat |= 0x01;
	set_irq(0, 0x20);  // INT_TIMER_1 (Bit 5 of Group 0)

	poll_wizfi_socket();

	if (m_t1_cmp > 0 && (m_t1_ctr & 0x01))
	{
		m_timer1->adjust(attotime::from_hz(60) * m_t1_cmp);
	}
}

uint8_t wildbits_jr2_state::timer_r(offs_t offset)
{
	switch (offset)
	{
	case 0x00: {
		uint8_t stat = m_t0_stat;
		m_t0_stat = 0; // Read clears status
		return stat;
	}
	case 0x01: return m_t0_val & 0xff;
	case 0x02: return (m_t0_val >> 8) & 0xff;
	case 0x03: return (m_t0_val >> 16) & 0xff;
	case 0x04: return m_t0_cmp_ctr;
	case 0x05: return m_t0_cmp & 0xff;
	case 0x06: return (m_t0_cmp >> 8) & 0xff;
	case 0x07: return (m_t0_cmp >> 16) & 0xff;
	case 0x08: {
		uint8_t stat = m_t1_stat;
		m_t1_stat = 0;
		return stat;
	}
	case 0x09: return m_t1_val & 0xff;
	case 0x0a: return (m_t1_val >> 8) & 0xff;
	case 0x0b: return (m_t1_val >> 16) & 0xff;
	case 0x0c: return m_t1_cmp_ctr;
	case 0x0d: return m_t1_cmp & 0xff;
	case 0x0e: return (m_t1_cmp >> 8) & 0xff;
	case 0x0f: return (m_t1_cmp >> 16) & 0xff;
	default: return 0;
	}
}

void wildbits_jr2_state::timer_w(offs_t offset, uint8_t data)
{
	switch (offset)
	{
	case 0x00: // T0_CTR
		m_t0_ctr = data;
		if (data & 0x01) // Enable
		{
			if (m_t0_cmp > 0)
			{
				attotime period = attotime::from_hz(25'175'000) * m_t0_cmp;
				if (period < attotime::from_hz(1000))
					period = attotime::from_hz(1000);
				m_timer0->adjust(period);
			}
		}
		else
		{
			m_timer0->adjust(attotime::never);
		}
		break;
	case 0x01: m_t0_val = (m_t0_val & 0xffff00) | data; break;
	case 0x02: m_t0_val = (m_t0_val & 0xff00ff) | (data << 8); break;
	case 0x03: m_t0_val = (m_t0_val & 0x00ffff) | (data << 16); break;
	case 0x04: m_t0_cmp_ctr = data; break;
	case 0x05: m_t0_cmp = (m_t0_cmp & 0xffff00) | data; break;
	case 0x06: m_t0_cmp = (m_t0_cmp & 0xff00ff) | (data << 8); break;
	case 0x07: m_t0_cmp = (m_t0_cmp & 0x00ffff) | (data << 16); break;
	case 0x08: // T1_CTR
		m_t1_ctr = data;
		if (data & 0x01)
		{
			if (m_t1_cmp > 0)
				m_timer1->adjust(attotime::from_hz(60) * m_t1_cmp);
		}
		else
		{
			m_timer1->adjust(attotime::never);
		}
		break;
	case 0x09: m_t1_val = (m_t1_val & 0xffff00) | data; break;
	case 0x0a: m_t1_val = (m_t1_val & 0xff00ff) | (data << 8); break;
	case 0x0b: m_t1_val = (m_t1_val & 0x00ffff) | (data << 16); break;
	case 0x0c: m_t1_cmp_ctr = data; break;
	case 0x0d: m_t1_cmp = (m_t1_cmp & 0xffff00) | data; break;
	case 0x0e: m_t1_cmp = (m_t1_cmp & 0xff00ff) | (data << 8); break;
	case 0x0f: m_t1_cmp = (m_t1_cmp & 0x00ffff) | (data << 16); break;
	}
}

// PS/2 Controller ($FE50 - $FE54)
void wildbits_jr2_state::queue_kbd_scancode(uint8_t scancode)
{
	m_kbd_fifo.push(scancode);
	set_irq(0, 0x04); // INT_PS2_KBD (Bit 2 of Group 0)
}

uint8_t wildbits_jr2_state::ps2_r(offs_t offset)
{
	switch (offset)
	{
	case 0x00: return m_ps2_ctrl;
	case 0x01: return m_ps2_out;
	case 0x02: // KBD_IN: Pop from FIFO
		if (!m_kbd_fifo.empty())
		{
			uint8_t code = m_kbd_fifo.front();
			m_kbd_fifo.pop();
			return code;
		}
		return 0;
	case 0x03: // MS_IN
		if (!m_mouse_fifo.empty())
		{
			uint8_t code = m_mouse_fifo.front();
			m_mouse_fifo.pop();
			return code;
		}
		return 0;
	case 0x04: { // PS2_STAT
		uint8_t stat = 0;
		if (m_kbd_fifo.empty()) stat |= 0x01;   // KEMP
		if (m_mouse_fifo.empty()) stat |= 0x02; // MEMP
		return stat;
	}
	default: return 0;
	}
}

void wildbits_jr2_state::ps2_w(offs_t offset, uint8_t data)
{
	switch (offset)
	{
	case 0x00: // PS2_CTRL
		m_ps2_ctrl = data;
		if (data & 0x10) // KCLR: Clear keyboard FIFO and IRQ
		{
			while (!m_kbd_fifo.empty()) m_kbd_fifo.pop();
			m_int_pending[0] &= ~0x04;
			check_irqs();
		}
		if (data & 0x20) // MCLR: Clear mouse FIFO and IRQ
		{
			while (!m_mouse_fifo.empty()) m_mouse_fifo.pop();
			m_int_pending[0] &= ~0x08;
			check_irqs();
		}
		if (data & 0x02) // Keyboard transmit trigger
		{
			if (m_ps2_out == 0xff) // Reset command
			{
				queue_kbd_scancode(0xfa); // ACK
				queue_kbd_scancode(0xaa); // BAT OK
			}
			else
			{
				queue_kbd_scancode(0xfa); // Default ACK
			}
		}
		if (data & 0x08) // Mouse transmit trigger
		{
			if (m_ps2_out == 0xff) // Reset command
			{
				m_mouse_fifo.push(0xfa); // ACK
				m_mouse_fifo.push(0xaa); // BAT OK
				m_mouse_fifo.push(0x00); // Mouse ID 0
				set_irq(0, 0x08);
			}
			else if (m_ps2_out == 0xf2) // Get ID
			{
				m_mouse_fifo.push(0xfa); // ACK
				m_mouse_fifo.push(0x00); // Standard PS/2 mouse ID
				set_irq(0, 0x08);
			}
			else
			{
				m_mouse_fifo.push(0xfa); // Default ACK
				set_irq(0, 0x08);
			}
		}
		break;
	case 0x01: // PS2_OUT
		m_ps2_out = data;
		break;
	}
}

// SPI SD Card Controller ($FE90 - $FE91)
void wildbits_jr2_state::sdcard_miso_w(int state)
{
	m_sdcard_miso = state;
}

uint8_t wildbits_jr2_state::sdc_stat_r()
{
	// Bit 0: CS_EN, Bit 1: SPI_CLK, Bit 7: 0 (not busy)
	return m_sdc_stat & 0x03;
}

void wildbits_jr2_state::sdc_stat_w(uint8_t data)
{
	m_sdc_stat = data;
	// Bit 0: CS_EN (1 = chip select active, 0 = inactive)
	m_sdcard->spi_ss_w((data & 0x01) ? 1 : 0);
}

uint8_t wildbits_jr2_state::sdc_data_r()
{
	return m_sdc_data_in;
}

void wildbits_jr2_state::sdc_data_w(uint8_t data)
{
	m_sdc_data_out = data;
	uint8_t in_byte = 0;
	for (int bit = 7; bit >= 0; bit--)
	{
		m_sdcard->spi_mosi_w((data >> bit) & 1);
		m_sdcard->spi_clock_w(1);
		in_byte = (in_byte << 1) | (m_sdcard_miso ? 1 : 0);
		m_sdcard->spi_clock_w(0);
	}
	m_sdc_data_in = in_byte;
}

// 16550 UART Serial Controller ($FE60 - $FE67)
void wildbits_jr2_state::poll_uart_socket()
{
	if (!m_uart_socket)
	{
		uint64_t filesize = 0;
		osd_file::open("socket.127.0.0.1:65504", OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_uart_socket, filesize);
	}
	if (!m_uart_socket)
		return;

	uint8_t buf[512];
	uint32_t actual = 0;
	std::error_condition err = m_uart_socket->read(buf, 0, sizeof(buf), actual);
	if (!err && actual > 0)
	{
		for (uint32_t i = 0; i < actual; i++)
		{
			if (m_uart_rx_fifo.size() < 4096)
				m_uart_rx_fifo.push(buf[i]);
		}
	}
}

uint8_t wildbits_jr2_state::uart_r(offs_t offset)
{
	poll_uart_socket();
	switch (offset & 7)
	{
	case 0: // RBR (or DLL if DLAB=1)
		if (m_uart_lcr & 0x80)
			return m_uart_dll;
		if (!m_uart_rx_fifo.empty())
		{
			uint8_t val = m_uart_rx_fifo.front();
			m_uart_rx_fifo.pop();
			return val;
		}
		return 0;
	case 1: // IER (or DLH if DLAB=1)
		if (m_uart_lcr & 0x80)
			return m_uart_dlh;
		return m_uart_ier;
	case 2: // IIR
		return m_uart_rx_fifo.empty() ? 0x01 : 0x04;
	case 3: // LCR
		return m_uart_lcr;
	case 4: // MCR
		return m_uart_mcr;
	case 5: // LSR
		return 0x60 | (m_uart_rx_fifo.empty() ? 0 : 0x01);
	case 6: // MSR
		return 0xb0; // DSR, CTS, DCD set
	case 7: // SCR
		return m_uart_scr;
	}
	return 0xff;
}

void wildbits_jr2_state::uart_w(offs_t offset, uint8_t data)
{
	poll_uart_socket();
	switch (offset & 7)
	{
	case 0: // THR (or DLL if DLAB=1)
		if (m_uart_lcr & 0x80)
		{
			m_uart_dll = data;
		}
		else
		{
			if (m_uart_socket)
			{
				uint32_t written = 0;
				m_uart_socket->write(&data, 0, 1, written);
			}
		}
		break;
	case 1: // IER (or DLH if DLAB=1)
		if (m_uart_lcr & 0x80)
			m_uart_dlh = data;
		else
			m_uart_ier = data;
		break;
	case 2: // FCR
		m_uart_fcr = data;
		if (data & 0x02) // Clear RX FIFO
		{
			while (!m_uart_rx_fifo.empty())
				m_uart_rx_fifo.pop();
		}
		break;
	case 3: // LCR
		m_uart_lcr = data;
		break;
	case 4: // MCR
		m_uart_mcr = data;
		break;
	case 5: // LSR
		break;
	case 6: // MSR
		break;
	case 7: // SCR
		m_uart_scr = data;
		break;
	}
}

// WizFi360 WiFi / SPI Controller ($FF20 - $FF2F)
void wildbits_jr2_state::push_wizfi_response(const std::string &resp)
{
	for (char c : resp)
	{
		if (m_wizfi_rx_fifo.size() < 2048)
			m_wizfi_rx_fifo.push((uint8_t)c);
	}
}

void wildbits_jr2_state::reset_wizfi()
{
	while (!m_wizfi_rx_fifo.empty())
		m_wizfi_rx_fifo.pop();
	m_wizfi_tx_buf.clear();
	m_wizfi_ctrl = 0;
	m_wizfi_transparent = false;
	m_wizfi_plus_count = 0;
	m_wizfi_cipsend_remaining = 0;
	m_wizfi_cipsend_total = 0;
	m_wizfi_cipmux = false;
	m_wizfi_cipmode = false;
	m_wizfi_echo = false;
	m_wizfi_cwmode = 1;
	m_wizfi_wifi_connected = true;
	m_wizfi_ssid = "WildbitsNet";
	m_wizfi_link_id = 0;
	m_wizfi_remote_host.clear();
	m_wizfi_remote_port = 0;
	if (m_wizfi_socket)
		m_wizfi_socket.reset();
}

void wildbits_jr2_state::poll_wizfi_socket()
{
	if (!m_wizfi_socket)
		return;

	uint8_t rx_buf[512];
	uint32_t actual = 0;
	std::error_condition err = m_wizfi_socket->read(rx_buf, 0, sizeof(rx_buf), actual);
	if (!err && actual > 0)
	{
		if (m_wizfi_transparent)
		{
			for (uint32_t i = 0; i < actual; i++)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push(rx_buf[i]);
			}
		}
		else
		{
			std::string header = m_wizfi_cipmux ?
				util::string_format("\r\n+IPD,%d,%d:", m_wizfi_link_id, actual) :
				util::string_format("\r\n+IPD,%d:", actual);
			for (char c : header)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push((uint8_t)c);
			}
			for (uint32_t i = 0; i < actual; i++)
			{
				if (m_wizfi_rx_fifo.size() < 2048)
					m_wizfi_rx_fifo.push(rx_buf[i]);
			}
		}
	}
}

void wildbits_jr2_state::handle_cipstart(const std::string &cmd)
{
	size_t eq = cmd.find('=');
	if (eq == std::string::npos)
	{
		push_wizfi_response("\r\nERROR\r\n");
		return;
	}

	std::string args = cmd.substr(eq + 1);
	std::vector<std::string> parts;
	std::string current;
	bool in_quote = false;
	for (char c : args)
	{
		if (c == '"')
		{
			in_quote = !in_quote;
		}
		else if (c == ',' && !in_quote)
		{
			parts.push_back(current);
			current.clear();
		}
		else
		{
			current += c;
		}
	}
	if (!current.empty())
		parts.push_back(current);

	int link_id = 0;
	std::string type, host;
	int port = 0;
	int idx = 0;

	if (parts.size() >= 3 && parts[0].find_first_not_of("0123456789") == std::string::npos && !parts[0].empty())
	{
		link_id = std::stoi(parts[0]);
		idx = 1;
	}

	if (idx < (int)parts.size()) type = parts[idx++];
	if (idx < (int)parts.size()) host = parts[idx++];
	if (idx < (int)parts.size()) port = std::stoi(parts[idx++]);

	while (!host.empty() && (host.front() == '"' || host.front() == ' ')) host.erase(0, 1);
	while (!host.empty() && (host.back() == '"' || host.back() == ' ')) host.pop_back();

	if (m_wizfi_socket)
	{
		push_wizfi_response("\r\nALREADY CONNECTED\r\n\r\nERROR\r\n");
		return;
	}

	if (!m_wizfi_wifi_connected)
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n", link_id));
		else
			push_wizfi_response("\r\nCLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n");
		return;
	}

	m_wizfi_link_id = link_id;
	m_wizfi_remote_host = host;
	m_wizfi_remote_port = port;

	uint64_t filesize = 0;
	std::string socket_path = util::string_format("socket.%s:%d", host.c_str(), port);
	std::error_condition err = osd_file::open(socket_path, OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_wizfi_socket, filesize);

	if (err && (host == "192.168.1.100" || host == "localhost"))
	{
		std::string fallback_path = util::string_format("socket.127.0.0.1:%d", port);
		err = osd_file::open(fallback_path, OPEN_FLAG_READ | OPEN_FLAG_WRITE, m_wizfi_socket, filesize);
	}

	if (!err && m_wizfi_socket)
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CONNECT\r\n\r\nOK\r\n", link_id));
		else
			push_wizfi_response("\r\nCONNECT\r\n\r\nOK\r\n");
	}
	else
	{
		if (m_wizfi_cipmux)
			push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n", link_id));
		else
			push_wizfi_response("\r\nCLOSED\r\n\r\nCONNECT FAIL\r\n\r\nERROR\r\n");
	}
}

void wildbits_jr2_state::handle_cipsend(const std::string &cmd)
{
	if (m_wizfi_cipmode)
	{
		push_wizfi_response("\r\nOK\r\n\r\n> ");
		m_wizfi_transparent = true;
		m_wizfi_plus_count = 0;
	}
	else
	{
		size_t eq = cmd.find('=');
		if (eq != std::string::npos)
		{
			std::string arg = cmd.substr(eq + 1);
			size_t comma = arg.find(',');
			int len = 0;
			if (comma != std::string::npos)
				len = std::stoi(arg.substr(comma + 1));
			else
				len = std::stoi(arg);

			m_wizfi_cipsend_remaining = len;
			m_wizfi_cipsend_total = len;
			push_wizfi_response("\r\nOK\r\n> ");
		}
		else
		{
			push_wizfi_response("\r\nERROR\r\n");
		}
	}
}

void wildbits_jr2_state::process_wizfi_cmd(const std::string &cmd_raw)
{
	if (m_wizfi_echo)
	{
		push_wizfi_response(cmd_raw + "\r\n");
	}

	std::string cmd = cmd_raw;
	while (!cmd.empty() && (cmd.back() == '\r' || cmd.back() == '\n' || cmd.back() == ' '))
		cmd.pop_back();

	if (cmd.empty())
		return;

	std::string cmd_upper = cmd;
	for (char &c : cmd_upper)
		c = toupper((unsigned char)c);

	if (cmd_upper == "AT")
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper == "ATE0")
	{
		m_wizfi_echo = false;
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper == "ATE1")
	{
		m_wizfi_echo = true;
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+GMR", 0) == 0)
	{
		push_wizfi_response("\r\nAT version:1.1.2.0(Apr 12 2023 08:08:36)\r\nSDK version:3.2.0(a0ffff9f)\r\ncompile time:Apr 12 2023 08:08:36\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+RST", 0) == 0)
	{
		reset_wizfi();
		push_wizfi_response("\r\nOK\r\n\r\nready\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+UART", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+UART_DEF" : "+UART_CUR";
			push_wizfi_response(util::string_format("\r\n%s:115200,8,1,0,0\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}

	else if (cmd_upper.rfind("AT+CWMODE", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWMODE_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWMODE_CUR" : "+CWMODE";
			push_wizfi_response(util::string_format("\r\n%s:%d\r\n\r\nOK\r\n", tag.c_str(), m_wizfi_cwmode));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos && eq + 1 < cmd.length())
				m_wizfi_cwmode = cmd[eq + 1] - '0';
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWDHCP", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWDHCP_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWDHCP_CUR" : "+CWDHCP";
			push_wizfi_response(util::string_format("\r\n%s:3\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWJAP", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CWJAP_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CWJAP_CUR" : "+CWJAP";
			push_wizfi_response(util::string_format("\r\n%s:\"%s\",\"00:08:dc:6b:e3:36\",1,-50\r\n\r\nOK\r\n", tag.c_str(), m_wizfi_ssid.c_str()));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos)
			{
				std::string arg = cmd.substr(eq + 1);
				size_t comma = arg.find(',');
				std::string ssid = (comma != std::string::npos) ? arg.substr(0, comma) : arg;
				while (!ssid.empty() && (ssid.front() == '"' || ssid.front() == ' ')) ssid.erase(0, 1);
				while (!ssid.empty() && (ssid.back() == '"' || ssid.back() == ' ')) ssid.pop_back();
				if (!ssid.empty())
					m_wizfi_ssid = ssid;
			}
			m_wizfi_wifi_connected = true;
			push_wizfi_response("\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CWQAP", 0) == 0)
	{
		m_wizfi_wifi_connected = false;
		push_wizfi_response("\r\nOK\r\nWIFI DISCONNECT\r\n");
	}
	else if (cmd_upper.rfind("AT+CWLAP", 0) == 0)
	{
		push_wizfi_response("\r\n+CWLAP:(4,\"WildbitsNet\",-50,\"00:08:dc:6b:e3:36\",1,0)\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPSTA", 0) == 0 && cmd_upper.find("MAC") == std::string::npos)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPSTA_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPSTA_CUR" : "+CIPSTA";
			push_wizfi_response(util::string_format("\r\n%s:ip:\"192.168.1.100\"\r\n%s:gateway:\"192.168.1.1\"\r\n%s:netmask:\"255.255.255.0\"\r\n\r\nOK\r\n",
				tag.c_str(), tag.c_str(), tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPSTAMAC", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPSTAMAC_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPSTAMAC_CUR" : "+CIPSTAMAC";
			push_wizfi_response(util::string_format("\r\n%s:\"00:08:dc:6b:e3:36\"\r\n\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPAPMAC", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			std::string tag = (cmd_upper.find("_DEF") != std::string::npos) ? "+CIPAPMAC_DEF" :
			                  (cmd_upper.find("_CUR") != std::string::npos) ? "+CIPAPMAC_CUR" : "+CIPAPMAC";
			push_wizfi_response(util::string_format("\r\n%s:\"02:08:dc:6b:e3:36\"\r\n\r\nOK\r\n", tag.c_str()));
		}
		else
		{
			push_wizfi_response("\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIFSR", 0) == 0)
	{
		push_wizfi_response("\r\n+CIFSR:STAIP,\"192.168.1.100\"\r\n+CIFSR:STAMAC,\"00:08:dc:6b:e3:36\"\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPMUX", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			push_wizfi_response(util::string_format("\r\n+CIPMUX:%d\r\n\r\nOK\r\n", m_wizfi_cipmux ? 1 : 0));
		}
		else
		{
			if (m_wizfi_socket)
			{
				push_wizfi_response("\r\nlink is builded\r\n\r\nERROR\r\n");
			}
			else
			{
				size_t eq = cmd.find('=');
				if (eq != std::string::npos && eq + 1 < cmd.length())
					m_wizfi_cipmux = (cmd[eq + 1] != '0');
				push_wizfi_response("\r\nOK\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPMODE", 0) == 0)
	{
		if (cmd.find('?') != std::string::npos)
		{
			push_wizfi_response(util::string_format("\r\n+CIPMODE:%d\r\n\r\nOK\r\n", m_wizfi_cipmode ? 1 : 0));
		}
		else
		{
			size_t eq = cmd.find('=');
			if (eq != std::string::npos && eq + 1 < cmd.length())
			{
				bool req_mode = (cmd[eq + 1] != '0');
				if (req_mode && m_wizfi_cipmux)
				{
					push_wizfi_response("\r\nERROR\r\n");
				}
				else
				{
					m_wizfi_cipmode = req_mode;
					push_wizfi_response("\r\nOK\r\n");
				}
			}
			else
			{
				push_wizfi_response("\r\nERROR\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPSTATUS", 0) == 0)
	{
		if (m_wizfi_socket)
		{
			push_wizfi_response(util::string_format("\r\nSTATUS:3\r\n+CIPSTATUS:%d,\"TCP\",\"%s\",%d,5000,0\r\n\r\nOK\r\n",
				m_wizfi_link_id, m_wizfi_remote_host.c_str(), m_wizfi_remote_port));
		}
		else if (m_wizfi_wifi_connected)
		{
			push_wizfi_response("\r\nSTATUS:2\r\n\r\nOK\r\n");
		}
		else
		{
			push_wizfi_response("\r\nSTATUS:5\r\n\r\nOK\r\n");
		}
	}
	else if (cmd_upper.rfind("AT+CIPSERVER", 0) == 0 || cmd_upper.rfind("AT+CIPSTO", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+CIPSTART", 0) == 0)
	{
		handle_cipstart(cmd);
	}
	else if (cmd_upper.rfind("AT+CIPCLOSE", 0) == 0)
	{
		if (m_wizfi_socket)
		{
			m_wizfi_socket.reset();
			m_wizfi_transparent = false;
			if (m_wizfi_cipmux)
				push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nOK\r\n", m_wizfi_link_id));
			else
				push_wizfi_response("\r\nCLOSED\r\n\r\nOK\r\n");
		}
		else
		{
			if (m_wizfi_cipmux)
			{
				size_t eq = cmd.find('=');
				int id = (eq != std::string::npos && eq + 1 < cmd.length()) ? (cmd[eq + 1] - '0') : m_wizfi_link_id;
				push_wizfi_response(util::string_format("\r\n%d,CLOSED\r\n\r\nERROR\r\n", id));
			}
			else
			{
				push_wizfi_response("\r\nERROR\r\n");
			}
		}
	}
	else if (cmd_upper.rfind("AT+CIPSEND", 0) == 0)
	{
		handle_cipsend(cmd);
	}
	else if (cmd_upper.rfind("AT+MQTTSET", 0) == 0 || cmd_upper.rfind("AT+MQTTTOPIC", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTCON", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTCON:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTPUB", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTPUB:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTSUB", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTSUB:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+MQTTDIS", 0) == 0)
	{
		push_wizfi_response("\r\n+MQTTDIS:OK\r\n\r\nOK\r\n");
	}
	else if (cmd_upper.rfind("AT+", 0) == 0)
	{
		push_wizfi_response("\r\nOK\r\n");
	}
	else
	{
		push_wizfi_response("\r\nERROR\r\n");
	}
}

uint8_t wildbits_jr2_state::wizfi_r(offs_t offset)
{
	switch (offset)
	{
	case 0x00: {
		uint8_t ctrl = m_wizfi_ctrl & 0x03;
		if (m_wizfi_tx_buf.empty() && m_wizfi_cipsend_remaining == 0) ctrl |= 0x08; // TxEmpty
		if (m_wizfi_rx_fifo.empty()) ctrl |= 0x04; // RxEmpty
		return ctrl;
	}
	case 0x01: {
		if (!m_wizfi_rx_fifo.empty())
		{
			uint8_t b = m_wizfi_rx_fifo.front();
			m_wizfi_rx_fifo.pop();
			return b;
		}
		return 0x00;
	}
	case 0x02: return 0x00; // Rx RD cnt hi
	case 0x03: return 0x00; // Rx RD cnt lo
	case 0x04: return (m_wizfi_rx_fifo.size() >> 8) & 0xff; // Rx WR cnt hi (available bytes)
	case 0x05: return m_wizfi_rx_fifo.size() & 0xff;        // Rx WR cnt lo
	case 0x06: return 0x00; // Tx RD cnt hi
	case 0x07: return 0x00; // Tx RD cnt lo
	case 0x08: return (m_wizfi_tx_buf.length() >> 8) & 0xff; // Tx WR cnt hi
	case 0x09: return m_wizfi_tx_buf.length() & 0xff;        // Tx WR cnt lo
	default: return 0x00;
	}
}

void wildbits_jr2_state::wizfi_w(offs_t offset, uint8_t data)
{
	switch (offset)
	{
	case 0x00: {
		bool was_in_reset = (m_wizfi_ctrl & 0x02) != 0;
		bool now_in_reset = (data & 0x02) != 0;
		m_wizfi_ctrl = data & 0x03;
		if (now_in_reset)
		{
			reset_wizfi();
			m_wizfi_ctrl |= 0x02;
		}
		else if (was_in_reset && !now_in_reset)
		{
			push_wizfi_response("\r\nready\r\nWIFI CONNECTED\r\nWIFI GOT IP\r\n");
		}
		break;
	}
	case 0x01:
		if (m_wizfi_transparent)
		{
			if (data == '+')
			{
				m_wizfi_plus_count++;
				if (m_wizfi_plus_count == 3)
				{
					m_wizfi_transparent = false;
					m_wizfi_plus_count = 0;
					return;
				}
			}
			else
			{
				if (m_wizfi_plus_count > 0)
				{
					for (int i = 0; i < m_wizfi_plus_count; i++)
					{
						char p = '+';
						uint32_t written = 0;
						if (m_wizfi_socket)
							m_wizfi_socket->write(&p, 0, 1, written);
					}
					m_wizfi_plus_count = 0;
				}
				if (m_wizfi_socket)
				{
					char c = (char)data;
					uint32_t written = 0;
					m_wizfi_socket->write(&c, 0, 1, written);
				}
			}
		}
		else if (m_wizfi_cipsend_remaining > 0)
		{
			if (m_wizfi_socket)
			{
				char c = (char)data;
				uint32_t written = 0;
				m_wizfi_socket->write(&c, 0, 1, written);
			}
			m_wizfi_cipsend_remaining--;
			if (m_wizfi_cipsend_remaining == 0)
			{
				std::string resp = util::string_format("\r\nRecv %d bytes\r\n\r\nSEND OK\r\n", m_wizfi_cipsend_total);
				push_wizfi_response(resp);
			}
		}
		else
		{
			if (data == '\r' || data == '\n')
			{
				if (!m_wizfi_tx_buf.empty())
				{
					process_wizfi_cmd(m_wizfi_tx_buf);
					m_wizfi_tx_buf.clear();
				}
			}
			else
			{
				m_wizfi_tx_buf += (char)data;
			}
		}
		break;
	}
}

// TinyVicky Master Registers ($FFC0 - $FFDF)
uint8_t wildbits_jr2_state::vky_r(offs_t offset)
{
	switch (offset)
	{
	case 0x00: return m_vky_mstr_ctrl_0;
	case 0x01: return m_vky_mstr_ctrl_1;
	case 0x02: return m_vky_layer_ctrl_0;
	case 0x03: return m_vky_layer_ctrl_1;
	case 0x04: return m_vky_brdr_ctrl;
	case 0x05: return m_vky_brdr_b;
	case 0x06: return m_vky_brdr_g;
	case 0x07: return m_vky_brdr_r;
	case 0x08: return m_vky_brdr_w;
	case 0x09: return m_vky_brdr_h;
	case 0x0d: return m_vky_bg_b;
	case 0x0e: return m_vky_bg_g;
	case 0x0f: return m_vky_bg_r;
	case 0x10: return m_vky_crsr_ctrl;
	case 0x12: return m_vky_crsr_char;
	case 0x13: return m_vky_crsr_color;
	case 0x14: return m_vky_crsr_x >> 8;
	case 0x15: return m_vky_crsr_x & 0xff;
	case 0x16: return m_vky_crsr_y >> 8;
	case 0x17: return m_vky_crsr_y & 0xff;
	default: return 0;
	}
}

void wildbits_jr2_state::vky_w(offs_t offset, uint8_t data)
{
	switch (offset)
	{
	case 0x00: m_vky_mstr_ctrl_0 = data; break;
	case 0x01: m_vky_mstr_ctrl_1 = data; break;
	case 0x02: m_vky_layer_ctrl_0 = data; break;
	case 0x03: m_vky_layer_ctrl_1 = data; break;
	case 0x04: m_vky_brdr_ctrl = data; break;
	case 0x05: m_vky_brdr_b = data; break;
	case 0x06: m_vky_brdr_g = data; break;
	case 0x07: m_vky_brdr_r = data; break;
	case 0x08: m_vky_brdr_w = data & 0x1f; break;
	case 0x09: m_vky_brdr_h = data & 0x1f; break;
	case 0x0d: m_vky_bg_b = data; break;
	case 0x0e: m_vky_bg_g = data; break;
	case 0x0f: m_vky_bg_r = data; break;
	case 0x10: m_vky_crsr_ctrl = data; break;
	case 0x12: m_vky_crsr_char = data; break;
	case 0x13: m_vky_crsr_color = data; break;
	case 0x14: m_vky_crsr_x = (m_vky_crsr_x & 0x00ff) | (data << 8); break;
	case 0x15: m_vky_crsr_x = (m_vky_crsr_x & 0xff00) | data; break;
	case 0x16: m_vky_crsr_y = (m_vky_crsr_y & 0x00ff) | (data << 8); break;
	case 0x17: m_vky_crsr_y = (m_vky_crsr_y & 0xff00) | data; break;
	default: break;
	}
}

void wildbits_jr2_state::wbjr2_mem(address_map &map)
{
	// Eight 8KB dynamic slots covering the entire 64KB logical address space
	map(0x0000, 0x1fff).bankr("bank0").bankw("bank0");
	map(0x2000, 0x3fff).bankr("bank1").bankw("bank1");
	map(0x4000, 0x5fff).bankr("bank2").bankw("bank2");
	map(0x6000, 0x7fff).bankr("bank3").bankw("bank3");
	map(0x8000, 0x9fff).bankr("bank4").bankw("bank4");
	map(0xa000, 0xbfff).bankr("bank5").bankw("bank5");
	map(0xc000, 0xdfff).bankr("bank6").bankw("bank6");
	map(0xe000, 0xffff).bankr("bank7").bankw("bank7");

	// Overlays in Slot 7 ($E000-$FFFF):
	// $FD00-$FDFF: Constant RAM for OS-9 Level 2 (when enabled in MMU_IO_CTRL bit 0)
	map(0xfd00, 0xfdff).lr8(NAME([this](offs_t offset) -> uint8_t {
		if (m_mmu_io_ctrl & 0x01)
			return m_constant_ram[offset];
		uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
		return get_physical_block_ptr(m_mlut[active_lut][7])[0x1d00 + offset];
	})).lw8(NAME([this](offs_t offset, uint8_t data) {
		if (m_mmu_io_ctrl & 0x01)
			m_constant_ram[offset] = data;
		else
		{
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			get_physical_block_ptr(m_mlut[active_lut][7])[0x1d00 + offset] = data;
		}
	}));

	// $FE00-$FE03: System Control & Software Reset registers
	map(0xfe00, 0xfe00).rw(FUNC(wildbits_jr2_state::sys0_r), FUNC(wildbits_jr2_state::sys0_w));
	map(0xfe01, 0xfe01).rw(FUNC(wildbits_jr2_state::sys1_r), FUNC(wildbits_jr2_state::sys1_w));
	map(0xfe02, 0xfe02).w(FUNC(wildbits_jr2_state::rst0_w));
	map(0xfe03, 0xfe03).w(FUNC(wildbits_jr2_state::rst1_w));

	// $FE07: Machine ID register
	map(0xfe07, 0xfe07).r(FUNC(wildbits_jr2_state::mid_r));

	// $FE10-$FE1F: Real-Time Clock (bq4802)
	map(0xfe10, 0xfe1f).rw(FUNC(wildbits_jr2_state::rtc_r), FUNC(wildbits_jr2_state::rtc_w));

	// $FE20-$FE2F: Interrupt Controller
	map(0xfe20, 0xfe2f).rw(FUNC(wildbits_jr2_state::intc_r), FUNC(wildbits_jr2_state::intc_w));

	// $FE30-$FE3F: System Timers 0 & 1
	map(0xfe30, 0xfe3f).rw(FUNC(wildbits_jr2_state::timer_r), FUNC(wildbits_jr2_state::timer_w));

	// $FE50-$FE54: PS/2 Keyboard and Mouse
	map(0xfe50, 0xfe54).rw(FUNC(wildbits_jr2_state::ps2_r), FUNC(wildbits_jr2_state::ps2_w));

	// $FE60-$FE67: 16550 UART (Serial / DriveWire)
	map(0xfe60, 0xfe67).rw(FUNC(wildbits_jr2_state::uart_r), FUNC(wildbits_jr2_state::uart_w));

	// $FE70-$FE72: Audio CODEC (WM8731)
	map(0xfe70, 0xfe72).rw(FUNC(wildbits_jr2_state::codec_r), FUNC(wildbits_jr2_state::codec_w));

	// $FE90-$FE91: SPI SD Card Controller
	map(0xfe90, 0xfe90).rw(FUNC(wildbits_jr2_state::sdc_stat_r), FUNC(wildbits_jr2_state::sdc_stat_w));
	map(0xfe91, 0xfe91).rw(FUNC(wildbits_jr2_state::sdc_data_r), FUNC(wildbits_jr2_state::sdc_data_w));

	// $FF20-$FF2F: WizFi360 WiFi / SPI Controller
	map(0xff20, 0xff2f).rw(FUNC(wildbits_jr2_state::wizfi_r), FUNC(wildbits_jr2_state::wizfi_w));

	// $FFA0-$FFA1: MMU Control registers
	map(0xffa0, 0xffa0).rw(FUNC(wildbits_jr2_state::mmu_mem_ctrl_r), FUNC(wildbits_jr2_state::mmu_mem_ctrl_w));
	map(0xffa1, 0xffa1).rw(FUNC(wildbits_jr2_state::mmu_io_ctrl_r), FUNC(wildbits_jr2_state::mmu_io_ctrl_w));

	// $FFA8-$FFAF: MMU Slot Assignment registers (when EDIT_EN=1)
	map(0xffa8, 0xffaf).rw(FUNC(wildbits_jr2_state::mmu_slot_r), FUNC(wildbits_jr2_state::mmu_slot_w));

	// $FFC0-$FFDF: TinyVicky Video Master Registers
	map(0xffc0, 0xffdf).rw(FUNC(wildbits_jr2_state::vky_r), FUNC(wildbits_jr2_state::vky_w));

	// $FFF0-$FFFF: Vector RAM overlay (when enabled in MMU_IO_CTRL bit 1)
	map(0xfff0, 0xffff).lr8(NAME([this](offs_t offset) -> uint8_t {
		if (m_mmu_io_ctrl & 0x02)
			return m_vector_ram[offset];
		uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
		return get_physical_block_ptr(m_mlut[active_lut][7])[0x1ff0 + offset];
	})).lw8(NAME([this](offs_t offset, uint8_t data) {
		if (m_mmu_io_ctrl & 0x02)
			m_vector_ram[offset] = data;
		else
		{
			uint8_t active_lut = m_mmu_mem_ctrl & 0x03;
			get_physical_block_ptr(m_mlut[active_lut][7])[0x1ff0 + offset] = data;
		}
	}));
}

uint32_t wildbits_jr2_state::screen_update(screen_device &screen, bitmap_rgb32 &bitmap, const rectangle &cliprect)
{
	// TinyVicky master video enable check
	bool text_en = (m_vky_mstr_ctrl_0 & 0x01) != 0;
	bool font_set1 = (m_vky_mstr_ctrl_1 & 0x20) != 0;
	uint16_t font_base = font_set1 ? 0x0800 : 0x0000;

	// Base background color
	rgb_t bg_clear(m_vky_bg_r, m_vky_bg_g, m_vky_bg_b);
	bitmap.fill(bg_clear, cliprect);

	if (text_en)
	{
		bool dbl_x = (m_vky_mstr_ctrl_1 & 0x02) != 0;
		bool dbl_y = (m_vky_mstr_ctrl_1 & 0x04) != 0;
		bool clk_70 = (m_vky_mstr_ctrl_1 & 0x01) != 0;

		const int cell_w = dbl_x ? 16 : 8;
		const int cell_h = dbl_y ? 16 : 8;

		const int cols = dbl_x ? 40 : 80;
		const int rows = dbl_y ? (clk_70 ? 25 : 30) : (clk_70 ? 50 : 60);

		const int x_scale = dbl_x ? 2 : 1;
		const int y_scale = dbl_y ? 2 : 1;

		for (int row = 0; row < rows; row++)
		{
			for (int col = 0; col < cols; col++)
			{
				int cell_idx = row * cols + col;
				uint8_t ch = m_vram_c2[cell_idx];
				uint8_t attr = m_vram_c3[cell_idx];

				uint8_t fg_idx = (attr >> 4) & 0x0f;
				uint8_t bg_idx = attr & 0x0f;

				// Palette lookup from Block $C0 (FG at $1700, BG at $1740)
				uint8_t fg_b = m_vram_c0[0x1700 + fg_idx * 4 + 0];
				uint8_t fg_g = m_vram_c0[0x1700 + fg_idx * 4 + 1];
				uint8_t fg_r = m_vram_c0[0x1700 + fg_idx * 4 + 2];
				rgb_t fg_pen(fg_r, fg_g, fg_b);

				uint8_t bg_b = m_vram_c0[0x1740 + bg_idx * 4 + 0];
				uint8_t bg_g = m_vram_c0[0x1740 + bg_idx * 4 + 1];
				uint8_t bg_r = m_vram_c0[0x1740 + bg_idx * 4 + 2];
				rgb_t bg_pen(bg_r, bg_g, bg_b);

				// Render character cell
				for (int cy = 0; cy < 8; cy++)
				{
					uint8_t glyph_row = m_vram_c1[font_base + ch * 8 + cy];

					for (int dy = 0; dy < y_scale; dy++)
					{
						int py = row * cell_h + cy * y_scale + dy;
						if (py > cliprect.max_y)
							break;
						if (py < cliprect.min_y)
							continue;

						uint32_t *dest = &bitmap.pix(py, col * cell_w);

						for (int cx = 0; cx < 8; cx++)
						{
							rgb_t pen = (glyph_row & (0x80 >> cx)) ? fg_pen : bg_pen;
							for (int dx = 0; dx < x_scale; dx++)
							{
								int px = col * cell_w + cx * x_scale + dx;
								if (px >= cliprect.min_x && px <= cliprect.max_x)
								{
									dest[cx * x_scale + dx] = pen;
								}
							}
						}
					}
				}
			}
		}

		// Render TinyVicky hardware cursor
		if (m_vky_crsr_ctrl & 0x01)
		{
			bool blink = (m_vky_crsr_ctrl & 0x02) ? (((m_frame_count / 16) & 1) == 0) : true;
			if (blink)
			{
				int crsr_col = m_vky_crsr_x;
				int crsr_row = m_vky_crsr_y;
				if (crsr_col >= 0 && crsr_col < cols && crsr_row >= 0 && crsr_row < rows)
				{
					for (int cy = 0; cy < cell_h; cy++)
					{
						int py = crsr_row * cell_h + cy;
						if (py >= cliprect.min_y && py <= cliprect.max_y)
						{
							uint32_t *dest = &bitmap.pix(py, crsr_col * cell_w);
							for (int cx = 0; cx < cell_w; cx++)
							{
								int px = crsr_col * cell_w + cx;
								if (px >= cliprect.min_x && px <= cliprect.max_x)
								{
									dest[cx] ^= 0x00ffffff;
								}
							}
						}
					}
				}
			}
		}
	}

	// Border rendering
	if (m_vky_brdr_ctrl & 0x01)
	{
		rgb_t brdr_pen(m_vky_brdr_r, m_vky_brdr_g, m_vky_brdr_b);
		int bw = m_vky_brdr_w;
		int bh = m_vky_brdr_h;

		if (bw > 0)
		{
			bitmap.plot_box(0, 0, bw, bitmap.height(), brdr_pen);
			bitmap.plot_box(bitmap.width() - bw, 0, bw, bitmap.height(), brdr_pen);
		}
		if (bh > 0)
		{
			bitmap.plot_box(0, 0, bitmap.width(), bh, brdr_pen);
			bitmap.plot_box(0, bitmap.height() - bh, bitmap.width(), bh, brdr_pen);
		}
	}

	return 0;
}

void wildbits_jr2_state::machine_start()
{
	m_ram = std::make_unique<uint8_t[]>(0x80000);     // 512KB SRAM
	m_vram_c0 = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $C0
	m_vram_c1 = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $C1 (Fonts & LUTs)
	m_vram_c2 = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $C2 (Text Matrix)
	m_vram_c3 = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $C3 (Color Matrix)
	m_vram_c4 = std::make_unique<uint8_t[]>(0x2000); // 8KB Block $C4 (Audio)

	m_timer0 = timer_alloc(FUNC(wildbits_jr2_state::timer0_tick), this);
	m_timer1 = timer_alloc(FUNC(wildbits_jr2_state::timer1_tick), this);

	save_pointer(NAME(m_ram), 0x80000);
	save_pointer(NAME(m_vram_c0), 0x2000);
	save_pointer(NAME(m_vram_c1), 0x2000);
	save_pointer(NAME(m_vram_c2), 0x2000);
	save_pointer(NAME(m_vram_c3), 0x2000);
	save_pointer(NAME(m_vram_c4), 0x2000);
	save_item(NAME(m_constant_ram));
	save_item(NAME(m_vector_ram));
	save_item(NAME(m_mlut));
	save_item(NAME(m_mmu_mem_ctrl));
	save_item(NAME(m_mmu_io_ctrl));
	save_item(NAME(m_sys0));
	save_item(NAME(m_sys1));
	save_item(NAME(m_rst0));
	save_item(NAME(m_rst1));
	save_item(NAME(m_int_pending));
	save_item(NAME(m_int_pol));
	save_item(NAME(m_int_edge));
	save_item(NAME(m_int_mask));
	save_item(NAME(m_t0_ctr));
	save_item(NAME(m_t0_stat));
	save_item(NAME(m_t0_val));
	save_item(NAME(m_t0_cmp));
	save_item(NAME(m_t0_cmp_ctr));
	save_item(NAME(m_t1_ctr));
	save_item(NAME(m_t1_stat));
	save_item(NAME(m_t1_val));
	save_item(NAME(m_t1_cmp));
	save_item(NAME(m_t1_cmp_ctr));
	save_item(NAME(m_ps2_ctrl));
	save_item(NAME(m_ps2_out));
	save_item(NAME(m_sdc_stat));
	save_item(NAME(m_sdc_data_in));
	save_item(NAME(m_sdc_data_out));
	save_item(NAME(m_sdcard_miso));
	save_item(NAME(m_vky_mstr_ctrl_0));
	save_item(NAME(m_vky_mstr_ctrl_1));
	save_item(NAME(m_vky_layer_ctrl_0));
	save_item(NAME(m_vky_layer_ctrl_1));
	save_item(NAME(m_vky_brdr_ctrl));
	save_item(NAME(m_vky_brdr_b));
	save_item(NAME(m_vky_brdr_g));
	save_item(NAME(m_vky_brdr_r));
	save_item(NAME(m_vky_brdr_w));
	save_item(NAME(m_vky_brdr_h));
	save_item(NAME(m_vky_bg_b));
	save_item(NAME(m_vky_bg_g));
	save_item(NAME(m_vky_bg_r));
	save_item(NAME(m_vky_crsr_ctrl));
	save_item(NAME(m_vky_crsr_char));
	save_item(NAME(m_vky_crsr_color));
	save_item(NAME(m_vky_crsr_x));
	save_item(NAME(m_vky_crsr_y));
}

void wildbits_jr2_state::machine_reset()
{
	printf("DEBUG: machine_reset() executed\n");
	// Default power-on Boot-from-Flash LUT configuration:
	// Slots 0..6 map to RAM blocks 0x00..0x06
	// Slot 7 maps to Flash block 0x7F (which contains reset vector $FFFE)
	for (int lut = 0; lut < 4; lut++)
	{
		for (int slot = 0; slot < 7; slot++)
		{
			m_mlut[lut][slot] = slot;
		}
		m_mlut[lut][7] = 0x7f; // High Flash ROM block
	}

	m_mmu_mem_ctrl = 0x00; // Active LUT 0, Edit LUT 0, EDIT_EN = 0
	m_mmu_io_ctrl = 0x00;  // Constant RAM and Vector RAM disabled
	m_sys0 = 0x00;
	m_sys1 = 0x00;
	m_rst0 = 0x00;
	m_rst1 = 0x00;

	// Reset Interrupt Controller (all IRQs masked by default)
	for (int g = 0; g < 4; g++)
	{
		m_int_pending[g] = 0x00;
		m_int_pol[g] = 0x00;
		m_int_edge[g] = 0x00;
		m_int_mask[g] = 0xff; // All masked
	}
	check_irqs();

	// Reset Timers
	m_t0_ctr = 0;
	m_t0_stat = 0;
	m_t0_val = 0;
	m_t0_cmp = 0;
	m_t0_cmp_ctr = 0;
	m_timer0->adjust(attotime::never);

	m_t1_ctr = 0;
	m_t1_stat = 0;
	m_t1_val = 0;
	m_t1_cmp = 0;
	m_t1_cmp_ctr = 0;
	m_timer1->adjust(attotime::never);

	// Reset PS/2 Controller
	m_ps2_ctrl = 0;
	m_ps2_out = 0;
	while (!m_kbd_fifo.empty()) m_kbd_fifo.pop();
	while (!m_mouse_fifo.empty()) m_mouse_fifo.pop();
	for (int i = 0; i < 4; i++)
		m_key_state[i] = 0;
	m_frame_count = 0;

	// Reset SDC Controller
	m_sdc_stat = 0x00;
	m_sdc_data_in = 0xff;
	m_sdc_data_out = 0xff;
	m_sdcard_miso = 1;

	// Reset WizFi360
	reset_wizfi();

	// Reset 16550 UART
	m_uart_dll = 0;
	m_uart_dlh = 0;
	m_uart_ier = 0;
	m_uart_fcr = 0;
	m_uart_lcr = 0;
	m_uart_mcr = 0;
	m_uart_scr = 0;
	while (!m_uart_rx_fifo.empty()) m_uart_rx_fifo.pop();

	// Initialize Video Master defaults: Text Mode enabled (640x480, 80x30)
	m_vky_mstr_ctrl_0 = 0x01; // TEXT enabled
	m_vky_mstr_ctrl_1 = 0x04; // 60Hz mode, DBL_Y enabled (80x30 default), Font 0
	m_vky_layer_ctrl_0 = 0x00;
	m_vky_layer_ctrl_1 = 0x00;
	m_vky_brdr_ctrl = 0x00;
	m_vky_brdr_b = 0x80;
	m_vky_brdr_g = 0x80;
	m_vky_brdr_r = 0x00;
	m_vky_brdr_w = 0;
	m_vky_brdr_h = 0;
	m_vky_bg_b = 0x00;
	m_vky_bg_g = 0x00;
	m_vky_bg_r = 0x00;
	m_vky_crsr_ctrl = 0x00;
	m_vky_crsr_char = 0x20;
	m_vky_crsr_color = 0xf0;
	m_vky_crsr_x = 0;
	m_vky_crsr_y = 0;

	memset(m_constant_ram, 0, sizeof(m_constant_ram));
	memset(m_vector_ram, 0, sizeof(m_vector_ram));
	memset(m_vram_c0.get(), 0, 0x2000);
	memset(m_vram_c1.get(), 0, 0x2000);
	memset(m_vram_c2.get(), 0x20, 0x2000); // Space filled text matrix
	memset(m_vram_c3.get(), 0xf0, 0x2000); // White on Black default color
	memset(m_vram_c4.get(), 0, 0x2000);

	// Load Default Font into Block $C1 (Font Set 0: Offset $0000-$07FF)
	for (int i = 0; i < 96; i++)
	{
		memcpy(&m_vram_c1[(32 + i) * 8], &s_default_font_8x8[i * 8], 8);
	}

	// Initialize Default Foreground and Background Palettes in Block $C1
	for (int i = 0; i < 16; i++)
	{
		rgb_t c = s_default_palette[i];
		// FG LUT at $1800 (Blue, Green, Red, Alpha)
		m_vram_c1[0x1800 + i * 4 + 0] = c.b();
		m_vram_c1[0x1800 + i * 4 + 1] = c.g();
		m_vram_c1[0x1800 + i * 4 + 2] = c.r();
		m_vram_c1[0x1800 + i * 4 + 3] = 0xff;

		// BG LUT at $1840 (Blue, Green, Red, Alpha)
		m_vram_c1[0x1840 + i * 4 + 0] = c.b();
		m_vram_c1[0x1840 + i * 4 + 1] = c.g();
		m_vram_c1[0x1840 + i * 4 + 2] = c.r();
		m_vram_c1[0x1840 + i * 4 + 3] = 0xff;
	}

	update_banks();
}

void wildbits_jr2_state::device_stop()
{
	printf("\n=== VRAM TEXT MATRIX SNAPSHOT ===\n");
	for (int r = 0; r < 25; r++)
	{
		char line[81];
		for (int c = 0; c < 80; c++)
		{
			uint8_t ch = m_vram_c2[r * 80 + c];
			line[c] = (ch >= 32 && ch < 127) ? ch : ' ';
		}
		line[80] = 0;
		printf("%02d: |%s|\n", r, line);
	}
	printf("=================================\n\n");
}

void wildbits_jr2_state::wbjr2(machine_config &config)
{
	// 6809 CPU clocked at 6.29 MHz (25.175 MHz / 4)
	MC6809(config, m_maincpu, XTAL(25'175'000));
	m_maincpu->set_addrmap(AS_PROGRAM, &wildbits_jr2_state::wbjr2_mem);

	// SPI SD Card Controller
	SPI_SDCARD(config, m_sdcard);
	m_sdcard->set_prefer_sdhc();
	m_sdcard->spi_miso_callback().set(FUNC(wildbits_jr2_state::sdcard_miso_w));

	// Video Screen: 640x480 @ 60Hz
	SCREEN(config, m_screen);
	m_screen->set_raw(XTAL(25'175'000), 800, 0, 640, 525, 0, 480);
	m_screen->set_screen_update(FUNC(wildbits_jr2_state::screen_update));
	m_screen->screen_vblank().set(FUNC(wildbits_jr2_state::vblank_w));
}

struct key_map_entry {
	uint8_t port;
	uint16_t mask;
	uint8_t scancode;
	bool extended;
};

static const key_map_entry s_key_map[] = {
	// Port 0: Letters A-P
	{ 0, 0x0001, 0x1c, false }, // A
	{ 0, 0x0002, 0x32, false }, // B
	{ 0, 0x0004, 0x21, false }, // C
	{ 0, 0x0008, 0x23, false }, // D
	{ 0, 0x0010, 0x24, false }, // E
	{ 0, 0x0020, 0x2b, false }, // F
	{ 0, 0x0040, 0x34, false }, // G
	{ 0, 0x0080, 0x33, false }, // H
	{ 0, 0x0100, 0x43, false }, // I
	{ 0, 0x0200, 0x3b, false }, // J
	{ 0, 0x0400, 0x42, false }, // K
	{ 0, 0x0800, 0x4b, false }, // L
	{ 0, 0x1000, 0x3a, false }, // M
	{ 0, 0x2000, 0x31, false }, // N
	{ 0, 0x4000, 0x44, false }, // O
	{ 0, 0x8000, 0x4d, false }, // P

	// Port 1: Letters Q-Z, Numbers 0-5
	{ 1, 0x0001, 0x15, false }, // Q
	{ 1, 0x0002, 0x2d, false }, // R
	{ 1, 0x0004, 0x1b, false }, // S
	{ 1, 0x0008, 0x2c, false }, // T
	{ 1, 0x0010, 0x3c, false }, // U
	{ 1, 0x0020, 0x2a, false }, // V
	{ 1, 0x0040, 0x1d, false }, // W
	{ 1, 0x0080, 0x22, false }, // X
	{ 1, 0x0100, 0x35, false }, // Y
	{ 1, 0x0200, 0x1a, false }, // Z
	{ 1, 0x0400, 0x45, false }, // 0
	{ 1, 0x0800, 0x16, false }, // 1
	{ 1, 0x1000, 0x1e, false }, // 2
	{ 1, 0x2000, 0x26, false }, // 3
	{ 1, 0x4000, 0x25, false }, // 4
	{ 1, 0x8000, 0x2e, false }, // 5

	// Port 2: Numbers 6-9, Enter, Space, Backspace, Tab, Esc, -, =, [, ], ;, ', backslash
	{ 2, 0x0001, 0x36, false }, // 6
	{ 2, 0x0002, 0x3d, false }, // 7
	{ 2, 0x0004, 0x3e, false }, // 8
	{ 2, 0x0008, 0x46, false }, // 9
	{ 2, 0x0010, 0x5a, false }, // Enter
	{ 2, 0x0020, 0x29, false }, // Space
	{ 2, 0x0040, 0x66, false }, // Backspace
	{ 2, 0x0080, 0x0d, false }, // Tab
	{ 2, 0x0100, 0x76, false }, // Escape
	{ 2, 0x0200, 0x4e, false }, // -
	{ 2, 0x0400, 0x55, false }, // =
	{ 2, 0x0800, 0x54, false }, // [
	{ 2, 0x1000, 0x5b, false }, // ]
	{ 2, 0x2000, 0x4c, false }, // ;
	{ 2, 0x4000, 0x52, false }, // '
	{ 2, 0x8000, 0x5d, false }, // Backslash

	// Port 3: ,, ., /, `, Shift, Ctrl, Alt, CapsLock, Arrows, Del
	{ 3, 0x0001, 0x41, false }, // ,
	{ 3, 0x0002, 0x49, false }, // .
	{ 3, 0x0004, 0x4a, false }, // /
	{ 3, 0x0008, 0x0e, false }, // `
	{ 3, 0x0010, 0x12, false }, // Left Shift
	{ 3, 0x0020, 0x59, false }, // Right Shift
	{ 3, 0x0040, 0x14, false }, // Left Ctrl
	{ 3, 0x0080, 0x11, false }, // Left Alt
	{ 3, 0x0100, 0x58, false }, // Caps Lock
	{ 3, 0x0200, 0x75, true  }, // Up Arrow
	{ 3, 0x0400, 0x72, true  }, // Down Arrow
	{ 3, 0x0800, 0x6b, true  }, // Left Arrow
	{ 3, 0x1000, 0x74, true  }, // Right Arrow
	{ 3, 0x2000, 0x71, true  }, // Delete
	{ 3, 0x4000, 0x6c, true  }, // Home
	{ 3, 0x8000, 0x69, true  }, // End
};

void wildbits_jr2_state::poll_keyboard()
{
	uint16_t port_vals[4];
	for (int p = 0; p < 4; p++)
	{
		port_vals[p] = m_io_key[p]->read();
	}

	for (const auto &entry : s_key_map)
	{
		bool is_down = (port_vals[entry.port] & entry.mask) != 0;
		bool was_down = (m_key_state[entry.port] & entry.mask) != 0;

		if (is_down && !was_down)
		{
			if (entry.extended)
				queue_kbd_scancode(0xe0);
			queue_kbd_scancode(entry.scancode);
		}
		else if (!is_down && was_down)
		{
			if (entry.extended)
				queue_kbd_scancode(0xe0);
			queue_kbd_scancode(0xf0);
			queue_kbd_scancode(entry.scancode);
		}
	}

	for (int p = 0; p < 4; p++)
	{
		m_key_state[p] = port_vals[p];
	}
}

void wildbits_jr2_state::vblank_w(int state)
{
	if (state)
	{
		m_frame_count++;
		poll_keyboard();
		set_irq(0, 0x01); // INT_VKY_SOF (Start of Frame / VSYNC 60Hz tick)
	}
}

static INPUT_PORTS_START( wbjr2 )
	PORT_START("KEY0")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('a') PORT_CHAR('A')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('b') PORT_CHAR('B')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('c') PORT_CHAR('C')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('d') PORT_CHAR('D')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('e') PORT_CHAR('E')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('f') PORT_CHAR('F')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('g') PORT_CHAR('G')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('h') PORT_CHAR('H')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('i') PORT_CHAR('I')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('j') PORT_CHAR('J')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('k') PORT_CHAR('K')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('l') PORT_CHAR('L')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('m') PORT_CHAR('M')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('n') PORT_CHAR('N')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('o') PORT_CHAR('O')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('p') PORT_CHAR('P')

	PORT_START("KEY1")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('q') PORT_CHAR('Q')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('r') PORT_CHAR('R')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('s') PORT_CHAR('S')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('t') PORT_CHAR('T')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('u') PORT_CHAR('U')
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('v') PORT_CHAR('V')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('w') PORT_CHAR('W')
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('x') PORT_CHAR('X')
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('y') PORT_CHAR('Y')
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('z') PORT_CHAR('Z')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_0) PORT_CHAR('0') PORT_CHAR(')')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('@')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')

	PORT_START("KEY2")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('^')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('&')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('*')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR('(')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Return") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ')
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB) PORT_CHAR(9)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Escape") PORT_CODE(KEYCODE_ESC) PORT_CHAR(27)
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('-') PORT_CHAR('_')
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('=') PORT_CHAR('+')
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('[') PORT_CHAR('{')
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR(']') PORT_CHAR('}')
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR(';') PORT_CHAR(':')
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('\'') PORT_CHAR('"')
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('\\') PORT_CHAR('|')

	PORT_START("KEY3")
	PORT_BIT( 0x0001, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR('<')
	PORT_BIT( 0x0002, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR('>')
	PORT_BIT( 0x0004, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('?')
	PORT_BIT( 0x0008, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('`') PORT_CHAR('~')
	PORT_BIT( 0x0010, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("LShift") PORT_CODE(KEYCODE_LSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0020, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RShift") PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x0040, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Ctrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
	PORT_BIT( 0x0080, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Alt") PORT_CODE(KEYCODE_LALT) PORT_CODE(KEYCODE_RALT)
	PORT_BIT( 0x0100, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Caps Lock") PORT_CODE(KEYCODE_CAPSLOCK) PORT_CHAR(UCHAR_MAMEKEY(CAPSLOCK))
	PORT_BIT( 0x0200, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Up") PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x0400, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x0800, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x1000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x2000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Delete") PORT_CODE(KEYCODE_DEL) PORT_CHAR(UCHAR_MAMEKEY(DEL))
	PORT_BIT( 0x4000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME) PORT_CHAR(UCHAR_MAMEKEY(HOME))
	PORT_BIT( 0x8000, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("End") PORT_CODE(KEYCODE_END) PORT_CHAR(UCHAR_MAMEKEY(END))
INPUT_PORTS_END

ROM_START(wbjr2)
	ROM_REGION(0x80000, "flash", ROMREGION_ERASEFF)
	ROM_LOAD("f0.dsk", 0x70000, 0x0a000, NO_DUMP)
	ROM_LOAD("booter", 0x7a000, 0x06000, NO_DUMP)
ROM_END

} // anonymous namespace

//    YEAR  NAME    PARENT  COMPAT  MACHINE  INPUT  CLASS               INIT        COMPANY     FULLNAME                       FLAGS
COMP( 2023, wbjr2,  0,      0,      wbjr2,   wbjr2, wildbits_jr2_state, empty_init, "Wildbits", "Wildbits Jr2 (FNX6809 Core)", MACHINE_NO_SOUND_HW )
