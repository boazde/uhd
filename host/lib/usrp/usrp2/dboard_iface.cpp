//
// Copyright 2010 Ettus Research LLC
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "usrp2_iface.hpp"
#include "clock_ctrl.hpp"
#include "usrp2_regs.hpp" //wishbone address constants
#include <uhd/usrp/dboard_iface.hpp>
#include <uhd/types/dict.hpp>
#include <uhd/utils/assert.hpp>
#include <boost/assign/list_of.hpp>
#include <boost/asio.hpp> //htonl and ntohl
#include <boost/math/special_functions/round.hpp>
#include "ad7922_regs.hpp" //aux adc
#include "ad5624_regs.hpp" //aux dac

using namespace uhd;
using namespace uhd::usrp;
using namespace boost::assign;

class usrp2_dboard_iface : public dboard_iface{
public:
    usrp2_dboard_iface(usrp2_iface::sptr iface, clock_ctrl::sptr clock_ctrl);
    ~usrp2_dboard_iface(void);

    void write_aux_dac(unit_t, int, float);
    float read_aux_adc(unit_t, int);

    void set_pin_ctrl(unit_t, boost::uint16_t);
    void set_atr_reg(unit_t, atr_reg_t, boost::uint16_t);
    void set_gpio_ddr(unit_t, boost::uint16_t);
    void write_gpio(unit_t, boost::uint16_t);
    boost::uint16_t read_gpio(unit_t);

    void write_i2c(boost::uint8_t, const byte_vector_t &);
    byte_vector_t read_i2c(boost::uint8_t, size_t);

    double get_clock_rate(unit_t);
    void set_clock_enabled(unit_t, bool);
    bool get_clock_enabled(unit_t);

    void write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    );

    boost::uint32_t read_write_spi(
        unit_t unit,
        const spi_config_t &config,
        boost::uint32_t data,
        size_t num_bits
    );

private:
    usrp2_iface::sptr _iface;
    clock_ctrl::sptr _clock_ctrl;
    boost::uint32_t _ddr_shadow;
    boost::uint32_t _gpio_shadow;

    uhd::dict<unit_t, ad5624_regs_t> _dac_regs;
    void _write_aux_dac(unit_t);
};

/***********************************************************************
 * Make Function
 **********************************************************************/
dboard_iface::sptr make_usrp2_dboard_iface(
    usrp2_iface::sptr iface,
    clock_ctrl::sptr clock_ctrl
){
    return dboard_iface::sptr(new usrp2_dboard_iface(iface, clock_ctrl));
}

/***********************************************************************
 * Structors
 **********************************************************************/
usrp2_dboard_iface::usrp2_dboard_iface(usrp2_iface::sptr iface, clock_ctrl::sptr clock_ctrl){
    _iface = iface;
    _clock_ctrl = clock_ctrl;
    _ddr_shadow = 0;
    _gpio_shadow = 0;

    //reset the aux dacs
    _dac_regs[UNIT_RX] = ad5624_regs_t();
    _dac_regs[UNIT_TX] = ad5624_regs_t();
    BOOST_FOREACH(unit_t unit, _dac_regs.keys()){
        _dac_regs[unit].data = 1;
        _dac_regs[unit].addr = ad5624_regs_t::ADDR_ALL;
        _dac_regs[unit].cmd  = ad5624_regs_t::CMD_RESET;
        this->_write_aux_dac(unit);
    }
}

usrp2_dboard_iface::~usrp2_dboard_iface(void){
    /* NOP */
}

/***********************************************************************
 * Clocks
 **********************************************************************/
double usrp2_dboard_iface::get_clock_rate(unit_t){
    return _iface->get_master_clock_freq();
}

void usrp2_dboard_iface::set_clock_enabled(unit_t unit, bool enb){
    switch(unit){
    case UNIT_RX: _clock_ctrl->enable_rx_dboard_clock(enb); return;
    case UNIT_TX: _clock_ctrl->enable_tx_dboard_clock(enb); return;
    }
}

/***********************************************************************
 * GPIO
 **********************************************************************/
static const uhd::dict<dboard_iface::unit_t, int> unit_to_shift = map_list_of
    (dboard_iface::UNIT_RX, 0)
    (dboard_iface::UNIT_TX, 16)
;

void usrp2_dboard_iface::set_pin_ctrl(unit_t unit, boost::uint16_t value){
    //calculate the new selection mux setting
    boost::uint32_t new_sels = 0x0;
    for(size_t i = 0; i < 16; i++){
        bool is_bit_set = bool(value & (0x1 << i));
        new_sels |= ((is_bit_set)? FRF_GPIO_SEL_ATR : FRF_GPIO_SEL_GPIO) << (i*2);
    }

    //write the selection mux value to register
    switch(unit){
    case UNIT_RX: _iface->poke32(FR_GPIO_RX_SEL, new_sels); return;
    case UNIT_TX: _iface->poke32(FR_GPIO_TX_SEL, new_sels); return;
    }
}

void usrp2_dboard_iface::set_gpio_ddr(unit_t unit, boost::uint16_t value){
    _ddr_shadow = \
        (_ddr_shadow & ~(0xffff << unit_to_shift[unit])) |
        (boost::uint32_t(value) << unit_to_shift[unit]);
    _iface->poke32(FR_GPIO_DDR, _ddr_shadow);
}

void usrp2_dboard_iface::write_gpio(unit_t unit, boost::uint16_t value){
    _gpio_shadow = \
        (_gpio_shadow & ~(0xffff << unit_to_shift[unit])) |
        (boost::uint32_t(value) << unit_to_shift[unit]);
    _iface->poke32(FR_GPIO_IO, _gpio_shadow);
}

boost::uint16_t usrp2_dboard_iface::read_gpio(unit_t unit){
    return boost::uint16_t(_iface->peek32(FR_GPIO_IO) >> unit_to_shift[unit]);
}

void usrp2_dboard_iface::set_atr_reg(unit_t unit, atr_reg_t atr, boost::uint16_t value){
    //define mapping of unit to atr regs to register address
    static const uhd::dict<
        unit_t, uhd::dict<atr_reg_t, boost::uint32_t>
    > unit_to_atr_to_addr = map_list_of
        (UNIT_RX, map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_RXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_RXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_RXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_RXSIDE)
        )
        (UNIT_TX, map_list_of
            (ATR_REG_IDLE,        FR_ATR_IDLE_TXSIDE)
            (ATR_REG_TX_ONLY,     FR_ATR_INTX_TXSIDE)
            (ATR_REG_RX_ONLY,     FR_ATR_INRX_TXSIDE)
            (ATR_REG_FULL_DUPLEX, FR_ATR_FULL_TXSIDE)
        )
    ;
    _iface->poke16(unit_to_atr_to_addr[unit][atr], value);
}

/***********************************************************************
 * SPI
 **********************************************************************/
static const uhd::dict<dboard_iface::unit_t, int> unit_to_spi_dev = map_list_of
    (dboard_iface::UNIT_TX, SPI_SS_TX_DB)
    (dboard_iface::UNIT_RX, SPI_SS_RX_DB)
;

void usrp2_dboard_iface::write_spi(
    unit_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    _iface->transact_spi(unit_to_spi_dev[unit], config, data, num_bits, false /*no rb*/);
}

boost::uint32_t usrp2_dboard_iface::read_write_spi(
    unit_t unit,
    const spi_config_t &config,
    boost::uint32_t data,
    size_t num_bits
){
    return _iface->transact_spi(unit_to_spi_dev[unit], config, data, num_bits, true /*rb*/);
}

/***********************************************************************
 * I2C
 **********************************************************************/
void usrp2_dboard_iface::write_i2c(boost::uint8_t addr, const byte_vector_t &bytes){
    return _iface->write_i2c(addr, bytes);
}

byte_vector_t usrp2_dboard_iface::read_i2c(boost::uint8_t addr, size_t num_bytes){
    return _iface->read_i2c(addr, num_bytes);
}

/***********************************************************************
 * Aux DAX/ADC
 **********************************************************************/
void usrp2_dboard_iface::_write_aux_dac(unit_t unit){
    static const uhd::dict<unit_t, int> unit_to_spi_dac = map_list_of
        (UNIT_RX, SPI_SS_RX_DAC)
        (UNIT_TX, SPI_SS_TX_DAC)
    ;
    _iface->transact_spi(
        unit_to_spi_dac[unit], spi_config_t::EDGE_FALL, 
        _dac_regs[unit].get_reg(), 24, false /*no rb*/
    );
}

void usrp2_dboard_iface::write_aux_dac(unit_t unit, int which, float value){
    _dac_regs[unit].data = boost::math::iround(4095*value/3.3);
    _dac_regs[unit].cmd = ad5624_regs_t::CMD_WR_UP_DAC_CHAN_N;
    switch(which){
    case 0: _dac_regs[unit].addr = ad5624_regs_t::ADDR_DAC_A; break;
    case 1: _dac_regs[unit].addr = ad5624_regs_t::ADDR_DAC_B; break;
    case 2: _dac_regs[unit].addr = ad5624_regs_t::ADDR_DAC_C; break;
    case 3: _dac_regs[unit].addr = ad5624_regs_t::ADDR_DAC_D; break;
    default: throw std::runtime_error("not a possible aux dac, must be 0, 1, 2, or 3");
    }
    this->_write_aux_dac(unit);
}

float usrp2_dboard_iface::read_aux_adc(unit_t unit, int which){
    static const uhd::dict<unit_t, int> unit_to_spi_adc = map_list_of
        (UNIT_RX, SPI_SS_RX_ADC)
        (UNIT_TX, SPI_SS_TX_ADC)
    ;

    //setup spi config args
    spi_config_t config;
    config.mosi_edge = spi_config_t::EDGE_FALL;
    config.miso_edge = spi_config_t::EDGE_RISE;

    //setup the spi registers
    ad7922_regs_t ad7922_regs;
    ad7922_regs.mod = which; //normal mode: mod == chn
    ad7922_regs.chn = which;

    //write and read spi
    _iface->transact_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16, false /*no rb*/
    );
    ad7922_regs.set_reg(boost::uint16_t(_iface->transact_spi(
        unit_to_spi_adc[unit], config,
        ad7922_regs.get_reg(), 16, true /*rb*/
    )));

    //convert to voltage and return
    return float(3.3*ad7922_regs.result/4095);
}
