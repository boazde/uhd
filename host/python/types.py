#
# Copyright 2017-2018 Ettus Research, a National Instruments Company
#
# SPDX-License-Identifier: GPL-3.0
#
""" @package types
Python UHD module containing types to be used with a MultiUSRP object
"""

from . import libpyuhd as lib


class StreamMode(lib.types.stream_mode):
    """See: uhd::stream_cmd_t::stream_mode_t"""
    pass


class StreamCMD(lib.types.stream_cmd):
    """See: uhd::stream_cmd_t"""
    pass


class TimeSpec(lib.types.time_spec):
    """See: uhd::time_spec_t"""
    pass


class SPIEdge(lib.types.spi_edge):
    """See: uhd::spi_config_t::spi_edge_t"""
    pass


class SPIConfig(lib.types.spi_config):
    """See: uhd::spi_config_t"""
    pass


class RXMetadataErrorCode(lib.types.rx_metadata_error_code):
    """See: uhd::rx_metadata_t::error_code_t"""
    pass


class Range(lib.types.range):
    """See: uhd::range_t"""
    pass


class RangeVector(lib.types.range_vector):
    """List of Ranges, ie vector<uhd::range_t>"""
    pass


class MetaRange(lib.types.meta_range):
    """See: uhd::meta_range_t"""
    pass


class RXMetadata(lib.types.rx_metadata):
    """See: uhd::rx_metadata_t"""
    pass


class TXMetadata(lib.types.tx_metadata):
    """See: uhd::tx_metadata_t"""
    pass


class TXAsyncMetadata(lib.types.async_metadata):
    """See: uhd::async_metadata"""
    pass


class TXMetadataEventCode(lib.types.tx_metadata_event_code):
    """See: uhd::async_metadata::event_code_t"""
    pass


class DataType(lib.types.data_type):
    """See: uhd::sensor_value_t::data_type_t"""
    pass


class SensorValue(lib.types.sensor_value):
    """See: uhd::sensor_value_t"""
    pass


class TuneRequestPolicy(lib.types.tune_request_policy):
    """See: uhd::tune_request_t::policy_t"""
    pass


class TuneRequest(lib.types.tune_request):
    """See: uhd::tune_request_t"""
    pass


class TuneResult(lib.types.tune_result):
    """See: uhd::tune_result_t"""
    pass
