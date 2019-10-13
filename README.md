# synapse
* =================================================================
* This file is part of "Synapse Project” (hereinafter the “Software”)
*
* =================================================================
* Developer(s):     Konstantin Bakanov, The Queen's University of Belfast, Northern Ireland.
*
* The project was funded by the Capital Markets Consortium.
*
* Copyright 2011-2019, The Queen's University of Belfast, Northern Ireland
* =================================================================
* Licensed under the GNU Lesser General Public License, as published by the Free Software Foundation, either version 3 or at your option any later version (hereinafter the "License");
* you must not use this Software except in compliance with the terms of the License.
*
* Unless required by applicable law or agreed upon in writing, this Software is distributed on an "AS IS" BASIS, WITHOUT ANY WARRANTY;
* without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, neither express nor implied.
* For details see terms of the License (see attached file: README. The License is also available at https://www.gnu.org/licenses/lgpl-3.0.en.html
* =================================================================

The Click project components are re-published under their own license, which may be found in the corresponding folder.

The main components of this project are the following:
- The click router and its elements. This can be compiled as standard. The example of the configure command is
  in the my_configure.sh file inside click-2.0.1 folder. Please make sure you have Linux kernel source code.
  This release has been tested with Vanilla Linux kernel 2.6.34.13.
  The installation commands are "make install-userlevel" and "make install-linuxmodule".
  If the linux module is too big to be loaded into the kernel, you can use "strip --strip-debug filename.ko"
  to reduce its size.
  Also, in kernel mode the ARP cache cannot get renewed, so make sure you fix the binding with something like:
  "sudo ip neighbor add 192.168.2.4 lladdr 08:00:27:3d:18:fb dev eth4 nud perm".
- The dev.c file in "kernel_modules/kernel_hack" needs to be put in place of the original dev.c file. It
  contains two hooks: one for the timestamper module and another one for the click router, when it runs
  in kernel mode.
- timestamper module in "kernel_modules/timestamper" needs to be built and loaded into the kernel.
  Please make sure you specify the correct network interface using the "s_netif_param" parameter.
  This module inserts timestamps into the trade messages for the purpose of benchmarking.
- msgSender, which can be found in "tools/tcp_probe", is responsible for sending
  trades as UDP messages. A sample script runMsgSender.sh is included.
- trades.txt in the data folder contains the sample trade messages.
- hand_coded folder contains the baseline versions of dmi, vortex and trix indicators.

Any other pieces of code are preserved as "useful side effects".
- 
