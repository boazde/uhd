#
#  Copyright 2017-2018 Ettus Research, a National Instruments Company
#
#  SPDX-License-Identifier: GPL-3.0
#
""" @package filters
Python UHD module containing the filter API
"""


from . import libpyuhd as lib


class FilterType(lib.filters.filter_type):
    """See: uhd::filter_info_base::filter_info_type"""
    pass


class FilterInfoBase(lib.filters.filter_info_base):
    """See: uhd::filter_info_base"""
    pass


class AnalogFilterBase(lib.filters.analog_filter_base):
    """See: uhd::analog_filter_base"""
    pass


class AnalogFilterLP(lib.filters.analog_filter_lp):
    """See: uhd::analog_filter_lp"""
    pass
