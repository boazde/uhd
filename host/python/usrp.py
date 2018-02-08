#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
""" @package usrp
Python UHD module containing the MultiUSRP and other objects
"""

import numpy as np
from . import libpyuhd as lib


class MultiUSRP(lib.usrp.multi_usrp):
    """
    MultiUSRP object for controlling devices
    """
    def __init__(self, args=""):
        """MultiUSRP constructor"""
        super(MultiUSRP, self).__init__(args)

    def recv_num_samps(self, num_samps, freq, rate=1e6, channels=(0,), gain=10):
        """
        RX a finite number of samples from the USRP
        :param num_samps: number of samples to RX
        :param freq: RX frequency (Hz)
        :param rate: RX sample rate (Hz)
        :param channels: list of channels to RX on
        :param gain: RX gain (dB)
        :return: numpy array of complex floating-point samples (fc32)
        """
        result = np.empty((len(channels), num_samps), dtype=np.complex64)

        for chan in channels:
            super(MultiUSRP, self).set_rx_rate(rate, chan)
            super(MultiUSRP, self).set_rx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_rx_gain(gain, chan)

        st_args = lib.usrp.stream_args("fc32", "sc16")
        st_args.channels = channels
        metadata = lib.types.rx_metadata()
        streamer = super(MultiUSRP, self).get_rx_stream(st_args)
        buffer_samps = streamer.get_max_num_samps()
        recv_buffer = np.zeros(
            (len(channels), buffer_samps), dtype=np.complex64)

        recv_samps = 0
        stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.start_cont)
        stream_cmd.stream_now = True
        streamer.issue_stream_cmd(stream_cmd)

        samps = np.array([], dtype=np.complex64)
        while recv_samps < num_samps:
            samps = streamer.recv(recv_buffer, metadata)

            if metadata.error_code != lib.types.rx_metadata_error_code.none:
                print(metadata.strerror())
            if samps:
                real_samps = min(num_samps - recv_samps, samps)
                result[:, recv_samps:recv_samps + real_samps] = recv_buffer[:, 0:real_samps]
                recv_samps += real_samps

        stream_cmd = lib.types.stream_cmd(lib.types.stream_mode.stop_cont)
        streamer.issue_stream_cmd(stream_cmd)

        while samps:
            samps = streamer.recv(recv_buffer, metadata)

        # Help the garbage collection
        streamer = None
        return result

    def send_waveform(self,
                      waveform_proto,
                      duration,
                      freq,
                      rate=1e6,
                      channels=(0,),
                      gain=10):
        """
        TX a finite number of samples from the USRP
        :param waveform_proto: numpy array of samples to TX
        :param duration: time in seconds to transmit at the supplied rate
        :param freq: TX frequency (Hz)
        :param rate: TX sample rate (Hz)
        :param channels: list of channels to TX on
        :param gain: TX gain (dB)
        :return: the number of transmitted samples
        """
        for chan in channels:
            super(MultiUSRP, self).set_tx_rate(rate, chan)
            super(MultiUSRP, self).set_tx_freq(lib.types.tune_request(freq), chan)
            super(MultiUSRP, self).set_tx_gain(gain, chan)

        st_args = lib.usrp.stream_args("fc32", "sc16")
        st_args.channels = channels

        metadata = lib.types.rx_metadata()
        streamer = super(MultiUSRP, self).get_tx_stream(st_args)
        buffer_samps = streamer.get_max_num_samps()
        proto_len = waveform_proto.shape[-1]

        if proto_len < buffer_samps:
            waveform_proto = np.tile(waveform_proto,
                                     (1, int(np.ceil(float(buffer_samps)/proto_len))))
            proto_len = waveform_proto.shape[-1]

        metadata = lib.types.tx_metadata()
        send_samps = 0
        max_samps = int(np.floor(duration * rate))

        if len(waveform_proto.shape) == 1:
            waveform_proto = waveform_proto.reshape(1, waveform_proto.size)
        if waveform_proto.shape[0] < len(channels):
            waveform_proto = np.tile(waveform_proto[0], (len(channels), 1))

        while send_samps < max_samps:
            real_samps = min(proto_len, max_samps-send_samps)
            if real_samps < proto_len:
                samples = streamer.send(waveform_proto[:real_samps], metadata)
            else:
                samples = streamer.send(waveform_proto, metadata)
            send_samps += samples

        # Help the garbage collection
        streamer = None
        return send_samps


class SubdevSpecPair(lib.usrp.subdev_spec_pair):
    """See: uhd::usrp::subdev_spec_pair_t"""
    pass


class SubdevSpec(lib.usrp.subdev_spec):
    """See: uhd::usrp::subdev_spec_t"""
    pass


class GPIOAtrReg(lib.usrp.gpio_atr_reg):
    """See: uhd::usrp::gpio_atr::gpio_atr_reg_t"""
    pass


class GPIOAtrMode(lib.usrp.gpio_atr_mode):
    """See: uhd::usrp::gpio_atr::gpio_atr_mode_t"""
    pass


class Unit(lib.usrp.unit):
    """See: uhd::usrp::dboard_iface::unit_t"""
    pass


class AuxDAC(lib.usrp.aux_dac):
    """See: uhd::usrp::dboard_iface::aux_dac_t"""
    pass


class AuxADC(lib.usrp.aux_adc):
    """See: uhd::usrp::dboard_iface::aux_adc_t"""
    pass


class SpecialProps(lib.usrp.special_props):
    """See: uhd::usrp::dboard_iface_special_props_t"""
    pass


class Sampling(lib.usrp.sampling):
    """See: uhd::usrp::fe_connection_t::sampling_t"""
    pass


class FEConnection(lib.usrp.fe_connection):
    """See: uhd::usrp::fe_connection_t"""
    pass


class StreamArgs(lib.usrp.stream_args):
    """See: uhd::stream_args_t"""
    pass


class RXStreamer(lib.usrp.rx_streamer):
    """See: uhd::rx_streamer"""
    pass


class TXStreamer(lib.usrp.tx_streamer):
    """See: uhd::tx_streamer"""
    pass
