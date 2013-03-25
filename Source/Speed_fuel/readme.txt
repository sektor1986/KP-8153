===============================================================================
                  Template Project for 966xx Series
===============================================================================
+-----------------------------------------------------------------------------+
| - The contents of this document are subject to change without notice.       |
|   Customers are advised to consult with sales representatives before        |
|   ordering.                                                                 |
| - The information, such as descriptions of function and application circuit |
|   examples, in this document are presented solely for the purpose of        |
|   reference to show examples of operations and uses of FUJITSU              |
|   SEMICONDUCTOR semiconductor device; FUJITSU SEMICONDUCTOR does not        |
|   warrant proper operation of the device with respect to use based on such  |
|   information. When you develop equipment incorporating the device based on |
|   such information, you must assume any responsibility arising out of such  |
|   use of the information.                                                   |
|   FUJITSU SEMICONDUCTOR assumes no liability for any damages whatsoever     |
|   arising out of the use of the information.                                |
| - Any information in this document, including descriptions of function and  |
|   schematic diagrams, shall not be construed as license of the use or       |
|   exercise of any intellectual property right, such as patent right or      |
|   copyright, or any other right of FUJITSU SEMICONDUCTOR or any third       |
|   party or does FUJITSU SEMICONDUCTOR warrant non-infringement of any       |
|   third-party's intellectual property right or other right by using such    |
|   information. FUJITSU SEMICONDUCTOR assumes no liability for any           |
|   infringement of the intellectual property rights or other rights of third |
|   parties which would result from the use of information contained herein.  |
| - The products described in this document are designed, developed and       |
|   manufactured as contemplated for general use, including without           |
|   limitation, ordinary industrial use, general office use, personal use,    |
|   and household use, but are not designed, developed and manufactured as    |
|   contemplated (1) for use accompanying fatal risks or dangers that, unless |
|   extremely high safety is secured, could have a serious effect to the      |
|   public, and could lead directly to death, personal injury, severe         |
|   physical damage or other loss (i.e., nuclear reaction control in nuclear  |
|   facility, aircraft flight control, air traffic control, mass transport    |
|   control, medical life support system, missile launch control in weapon    |
|   system), or (2) for use requiring extremely high reliability              |
|   (i.e., submersible repeater and artificial satellite). Please note that   |
|   FUJITSU SEMICONDUCTOR will not be liable against you and/or any third     |
|   party for any claims or damages arising in connection with                |
|   above-mentioned uses of the products.                                     |
| - Any semiconductor devices have an inherent chance of failure. You must    |
|   protect against injury, damage or loss from such failures by              |
|   incorporating safety design measures into your facility and equipment     |
|   such as redundancy, fire protection, and prevention of over-current       |
|   levels and other abnormal operating conditions.                           |
| - Exportation/release of any products described in this document may        |
|   require necessary procedures in accordance with the regulations of the    |
|   Foreign Exchange and Foreign Trade Control Law of Japan and/or US export  |
|   control laws.                                                             |
+-----------------------------------------------------------------------------+
===============================================================================
1. introduction
   This is a project template for the MB966xx Series. It includes some basic
   settings for e.g. Linker, C-Compiler which must be checked and modified in
   detail, corresponding to the user application and device type.

===============================================================================
2. target device
   device : MB96F673AB

===============================================================================
3. file composition
   readme.txt     : This file ; revision and history of this project
   Template files : start.asm, main.c, vectors.c, romconst.prc,
                    ChangeHighSpeed_before4.prc, ChangeHighSpeed_before8.prc,
                    ChangeHighSpeed_after.prc
                    history.txt  (revision and history of Template files)
   I/O register files : _ffmc16.c, _ffmc16.h, mb966xx.h
   io_reg.txt     : revision and history of I/O register files

===============================================================================
4. The main settings of the project
  - The warning Level of "start.asm" is individually set to level0.
  - The compilation option (-Xinittbl) is individually set to _ffmc16.C.

===============================================================================
5. usage environment
   Please use the following or later versions when you use this template.
   Workbench  : V30L36
   C Compiler : V30L18
   Assembler  : V30L15

===============================================================================
6. History
Date        Ver  Description
2011-10-26  1.0  first edition
2012-01-12  1.1  changed start.asm
                 added Flash load high speed procedure
2012-04-13  1.2  Update according to change of start.asm
2012-07-26  1.3  Update according to change of start.asm and I/O register file
2012-12-26  1.4 -Change of start.asm
                 -addition 5.13(Declaration of RAMCODE section and labels)
                -Update of I/O register files
