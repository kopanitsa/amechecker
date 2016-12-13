#
# Copyright 2014, Broadcom Corporation
# All Rights Reserved.
#
# This is UNPUBLISHED PROPRIETARY SOURCE CODE of Broadcom Corporation;
# the contents of this file may not be disclosed to third parties, copied
# or duplicated in any form, in whole or in part, without the prior
# written permission of Broadcom Corporation.
#

########################################################################
# Add Application sources here.
########################################################################
APP_SRC = ame.c log.c

########################################################################
################ DO NOT MODIFY FILE BELOW THIS LINE ####################
########################################################################
APP_PATCHES_AND_LIBS += bt_clock_based_periodic_timer.a
