EESchema Schematic File Version 4
EELAYER 30 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 1 2
Title ""
Date ""
Rev "1"
Comp ""
Comment1 "Designed for AISLER 2-Layer Service"
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
$Sheet
S 4650 1550 1150 700 
U 5F189B7D
F0 "Universal VFD PSU by Rolo Kamp" 50
F1 "universal_vfd_psu.sch" 50
F2 "5V" I L 4650 1600 50 
F3 "HV" O R 5800 1750 50 
F4 "~HV_SHDN~" I L 4650 1850 50 
F5 "GND" I L 4650 2200 50 
F6 "FIL_SHDN" I L 4650 1950 50 
F7 "FIL_B" O R 5800 2050 50 
F8 "FIL_A" O R 5800 1950 50 
$EndSheet
$Comp
L IVL2-7_5:IVL2-5_7 U1
U 1 1 5F192B05
P 9300 2100
F 0 "U1" H 10181 2171 50  0000 L CNN
F 1 "IVL2-5_7" H 10181 2080 50  0000 L CNN
F 2 "extra_libraries:IVL2-7_5" H 8350 2350 50  0001 C CNN
F 3 "" H 8350 2350 50  0001 C CNN
	1    9300 2100
	1    0    0    -1  
$EndComp
Text GLabel 5850 2050 2    50   Input ~ 0
FILAMENT+
Text GLabel 5850 1950 2    50   Input ~ 0
FILAMENT-
Text GLabel 8550 1500 1    50   Input ~ 0
FILAMENT+
Text GLabel 8500 2700 3    50   Input ~ 0
FILAMENT+
Text GLabel 10050 1500 1    50   Input ~ 0
FILAMENT-
Text GLabel 10100 2700 3    50   Input ~ 0
FILAMENT-
Wire Wire Line
	5850 1950 5800 1950
Wire Wire Line
	5800 2050 5850 2050
$Comp
L HV5812:HV5812WG U4
U 1 1 5F19BBE0
P 9400 3550
F 0 "U4" H 9425 3615 50  0000 C CNN
F 1 "HV5812WG" H 9425 3524 50  0000 C CNN
F 2 "Package_SO:SOIC-28W_7.5x17.9mm_P1.27mm" H 9400 3550 50  0001 C CNN
F 3 "http://www.microchip.com/mymicrochip/filehandler.aspx?ddocname=en570713" H 9400 3550 50  0001 C CNN
	1    9400 3550
	1    0    0    -1  
$EndComp
$Comp
L power:+24V #PWR0105
U 1 1 5F1A10B2
P 5900 1750
F 0 "#PWR0105" H 5900 1600 50  0001 C CNN
F 1 "+24V" H 5915 1923 50  0000 C CNN
F 2 "" H 5900 1750 50  0001 C CNN
F 3 "" H 5900 1750 50  0001 C CNN
	1    5900 1750
	0    1    1    0   
$EndComp
$Comp
L power:+24V #PWR0106
U 1 1 5F1A16B0
P 8800 3750
F 0 "#PWR0106" H 8800 3600 50  0001 C CNN
F 1 "+24V" H 8815 3923 50  0000 C CNN
F 2 "" H 8800 3750 50  0001 C CNN
F 3 "" H 8800 3750 50  0001 C CNN
	1    8800 3750
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR0107
U 1 1 5F1A1AD8
P 8550 3750
F 0 "#PWR0107" H 8550 3600 50  0001 C CNN
F 1 "+5V" H 8565 3923 50  0000 C CNN
F 2 "" H 8550 3750 50  0001 C CNN
F 3 "" H 8550 3750 50  0001 C CNN
	1    8550 3750
	1    0    0    -1  
$EndComp
Wire Wire Line
	8800 3750 8800 3800
Wire Wire Line
	8800 3800 8900 3800
Wire Wire Line
	8900 3900 8550 3900
Wire Wire Line
	8550 3900 8550 3750
$Comp
L power:GND #PWR0108
U 1 1 5F1A2A0D
P 8650 5750
F 0 "#PWR0108" H 8650 5500 50  0001 C CNN
F 1 "GND" H 8655 5577 50  0000 C CNN
F 2 "" H 8650 5750 50  0001 C CNN
F 3 "" H 8650 5750 50  0001 C CNN
	1    8650 5750
	1    0    0    -1  
$EndComp
Entry Wire Line
	10550 5500 10650 5400
Entry Wire Line
	10550 5600 10650 5500
Entry Wire Line
	10550 5700 10650 5600
Entry Wire Line
	10550 5400 10650 5300
Entry Wire Line
	10550 5300 10650 5200
Entry Wire Line
	10550 5200 10650 5100
Entry Wire Line
	10550 5100 10650 5000
Entry Wire Line
	10550 5000 10650 4900
Entry Wire Line
	10550 4900 10650 4800
Entry Wire Line
	10550 4800 10650 4700
Entry Wire Line
	10550 4700 10650 4600
Entry Wire Line
	10550 4600 10650 4500
Entry Wire Line
	8550 3250 8650 3150
Entry Wire Line
	8650 3250 8750 3150
Entry Wire Line
	8750 3250 8850 3150
Entry Wire Line
	8850 3250 8950 3150
Entry Wire Line
	8950 3250 9050 3150
Entry Wire Line
	9050 3250 9150 3150
Entry Wire Line
	9150 3250 9250 3150
Entry Wire Line
	9250 3250 9350 3150
Entry Wire Line
	9350 3250 9450 3150
Entry Wire Line
	9450 3250 9550 3150
Entry Wire Line
	9550 3250 9650 3150
Entry Wire Line
	9650 3250 9750 3150
Entry Wire Line
	9750 3250 9850 3150
Entry Wire Line
	9850 3250 9950 3150
Entry Wire Line
	10550 4500 10650 4400
Entry Wire Line
	10550 4400 10650 4300
Wire Wire Line
	8650 2650 8650 3150
Wire Wire Line
	8750 3150 8750 2650
Wire Wire Line
	8850 2650 8850 3150
Wire Wire Line
	8950 3150 8950 2650
Wire Wire Line
	9050 2650 9050 3150
Wire Wire Line
	9150 3150 9150 2650
Wire Wire Line
	9250 2650 9250 3150
Wire Wire Line
	9350 3150 9350 2650
Wire Wire Line
	9450 3150 9450 2650
Wire Wire Line
	9550 2650 9550 3150
Wire Wire Line
	9650 3150 9650 2650
Wire Wire Line
	9750 2650 9750 3150
Wire Wire Line
	9850 3150 9850 2650
Wire Wire Line
	9950 2650 9950 3150
Wire Wire Line
	10050 2650 10100 2650
Wire Wire Line
	10100 2650 10100 2700
Wire Wire Line
	8550 2650 8500 2650
Wire Wire Line
	8500 2650 8500 2700
Wire Wire Line
	5900 1750 5800 1750
$Comp
L power:+5V #PWR0109
U 1 1 5F23255C
P 4550 1550
F 0 "#PWR0109" H 4550 1400 50  0001 C CNN
F 1 "+5V" H 4565 1723 50  0000 C CNN
F 2 "" H 4550 1550 50  0001 C CNN
F 3 "" H 4550 1550 50  0001 C CNN
	1    4550 1550
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 1550 4550 1600
Wire Wire Line
	4550 1600 4650 1600
$Comp
L power:GND #PWR0110
U 1 1 5F23341A
P 4550 2250
F 0 "#PWR0110" H 4550 2000 50  0001 C CNN
F 1 "GND" H 4555 2077 50  0000 C CNN
F 2 "" H 4550 2250 50  0001 C CNN
F 3 "" H 4550 2250 50  0001 C CNN
	1    4550 2250
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 2250 4550 2200
Wire Wire Line
	4550 2200 4650 2200
Text Label 8650 3100 1    50   ~ 0
G1
Text Label 8750 3100 1    50   ~ 0
dt
Text Label 8850 3100 1    50   ~ 0
g
Text Label 8950 3100 1    50   ~ 0
e
Text Label 9050 3100 1    50   ~ 0
G2
Text Label 9150 3100 1    50   ~ 0
c
Text Label 9250 3100 1    50   ~ 0
G3
Text Label 9350 3100 1    50   ~ 0
db
Text Label 9450 3100 1    50   ~ 0
d
Text Label 9550 3100 1    50   ~ 0
G4
Text Label 9650 3100 1    50   ~ 0
b
Text Label 9750 3100 1    50   ~ 0
f
Text Label 9850 3100 1    50   ~ 0
a
Text Label 9950 3100 1    50   ~ 0
G5
Text Label 10150 4400 2    50   ~ 0
G1
NoConn ~ 9950 4300
NoConn ~ 9950 4200
NoConn ~ 9950 4100
NoConn ~ 9950 4000
NoConn ~ 9950 3900
NoConn ~ 9950 3800
Text Label 10150 4500 2    50   ~ 0
dt
Text Label 10150 4600 2    50   ~ 0
g
Text Label 10150 4700 2    50   ~ 0
e
Text Label 10150 5700 2    50   ~ 0
G2
Text Label 10150 5600 2    50   ~ 0
c
Text Label 10150 5500 2    50   ~ 0
G3
Text Label 10150 5400 2    50   ~ 0
db
Text Label 10150 5300 2    50   ~ 0
d
Text Label 10150 5200 2    50   ~ 0
G4
Text Label 10150 5100 2    50   ~ 0
b
Text Label 10150 5000 2    50   ~ 0
f
Text Label 10150 4900 2    50   ~ 0
a
Text Label 10150 4800 2    50   ~ 0
G5
Wire Wire Line
	9950 5700 10550 5700
Wire Wire Line
	9950 5600 10550 5600
Wire Wire Line
	9950 5500 10550 5500
Wire Wire Line
	9950 5400 10550 5400
Wire Wire Line
	9950 5300 10550 5300
Wire Wire Line
	9950 5200 10550 5200
Wire Wire Line
	9950 5100 10550 5100
Wire Wire Line
	9950 5000 10550 5000
Wire Wire Line
	9950 4900 10550 4900
Wire Wire Line
	9950 4800 10550 4800
Wire Wire Line
	9950 4700 10550 4700
Wire Wire Line
	9950 4600 10550 4600
Wire Wire Line
	9950 4500 10550 4500
Wire Wire Line
	9950 4400 10550 4400
$Comp
L Connector:Conn_01x08_Male J1
U 1 1 5F272926
P 2050 2000
F 0 "J1" H 2022 1882 50  0000 R CNN
F 1 "Connection Header" H 2022 1973 50  0000 R CNN
F 2 "Connector_PinHeader_2.54mm:PinHeader_1x08_P2.54mm_Vertical" H 2050 2000 50  0001 C CNN
F 3 "~" H 2050 2000 50  0001 C CNN
	1    2050 2000
	-1   0    0    1   
$EndComp
Text GLabel 4450 1850 0    50   Input ~ 0
~HV_SHDN~
Text GLabel 4450 1950 0    50   Input ~ 0
FIL_SHDN
Wire Wire Line
	4450 1850 4650 1850
Wire Wire Line
	4450 1950 4650 1950
$Comp
L power:GND #PWR011
U 1 1 5F277F56
P 1750 2350
F 0 "#PWR011" H 1750 2100 50  0001 C CNN
F 1 "GND" H 1755 2177 50  0000 C CNN
F 2 "" H 1750 2350 50  0001 C CNN
F 3 "" H 1750 2350 50  0001 C CNN
	1    1750 2350
	1    0    0    -1  
$EndComp
Wire Wire Line
	1750 2350 1750 2300
Wire Wire Line
	1750 2300 1850 2300
$Comp
L power:+5V #PWR010
U 1 1 5F27A934
P 1550 2350
F 0 "#PWR010" H 1550 2200 50  0001 C CNN
F 1 "+5V" H 1565 2523 50  0000 C CNN
F 2 "" H 1550 2350 50  0001 C CNN
F 3 "" H 1550 2350 50  0001 C CNN
	1    1550 2350
	-1   0    0    1   
$EndComp
Wire Wire Line
	1550 2350 1550 2200
Wire Wire Line
	1550 2200 1850 2200
Text GLabel 1800 1600 0    50   Input ~ 0
~HV_SHDN~
Text GLabel 1800 1700 0    50   Input ~ 0
FIL_SHDN
Wire Wire Line
	1800 1600 1850 1600
Text GLabel 8500 4750 0    50   Input ~ 0
HV_CLOCK
Text GLabel 8500 4850 0    50   Input ~ 0
HV_DATA
Text GLabel 8500 4950 0    50   Input ~ 0
HV_STROBE
NoConn ~ 8900 5050
$Comp
L Device:R_Small R14
U 1 1 5F2820E7
P 8650 5400
F 0 "R14" H 8709 5446 50  0000 L CNN
F 1 "4.7kΩ" H 8709 5355 50  0000 L CNN
F 2 "Resistor_SMD:R_0805_2012Metric_Pad1.15x1.40mm_HandSolder" H 8650 5400 50  0001 C CNN
F 3 "~" H 8650 5400 50  0001 C CNN
	1    8650 5400
	1    0    0    -1  
$EndComp
Wire Wire Line
	8650 5700 8650 5750
Wire Wire Line
	8650 5700 8900 5700
Wire Wire Line
	8650 5700 8650 5500
Connection ~ 8650 5700
Wire Wire Line
	8650 5300 8650 5150
Wire Wire Line
	8650 5150 8900 5150
Text GLabel 8500 5150 0    50   Input ~ 0
HV_BLANK
Wire Wire Line
	8500 5150 8650 5150
Connection ~ 8650 5150
Wire Wire Line
	8900 4950 8500 4950
Wire Wire Line
	8500 4850 8900 4850
Wire Wire Line
	8900 4750 8500 4750
Text GLabel 1800 2100 0    50   Input ~ 0
HV_CLOCK
Text GLabel 1800 2000 0    50   Input ~ 0
HV_DATA
Text GLabel 1800 1900 0    50   Input ~ 0
HV_STROBE
Text GLabel 1800 1800 0    50   Input ~ 0
HV_BLANK
Wire Wire Line
	1800 1800 1850 1800
Wire Wire Line
	1800 1900 1850 1900
Wire Wire Line
	1800 2000 1850 2000
Wire Wire Line
	1800 2100 1850 2100
Wire Wire Line
	1850 1700 1800 1700
$Comp
L Mechanical:MountingHole_Pad H1
U 1 1 5F2AB557
P 1600 3000
F 0 "H1" V 1554 3150 50  0000 L CNN
F 1 "MountingHole_Pad" V 1645 3150 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5_Pad" H 1600 3000 50  0001 C CNN
F 3 "~" H 1600 3000 50  0001 C CNN
	1    1600 3000
	0    1    1    0   
$EndComp
$Comp
L Mechanical:MountingHole_Pad H2
U 1 1 5F2ABB49
P 1600 3200
F 0 "H2" V 1554 3350 50  0000 L CNN
F 1 "MountingHole_Pad" V 1645 3350 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5_Pad" H 1600 3200 50  0001 C CNN
F 3 "~" H 1600 3200 50  0001 C CNN
	1    1600 3200
	0    1    1    0   
$EndComp
$Comp
L Mechanical:MountingHole_Pad H3
U 1 1 5F2ABD15
P 1600 3400
F 0 "H3" V 1554 3550 50  0000 L CNN
F 1 "MountingHole_Pad" V 1645 3550 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5_Pad" H 1600 3400 50  0001 C CNN
F 3 "~" H 1600 3400 50  0001 C CNN
	1    1600 3400
	0    1    1    0   
$EndComp
$Comp
L Mechanical:MountingHole_Pad H4
U 1 1 5F2ABF37
P 1600 3600
F 0 "H4" V 1554 3750 50  0000 L CNN
F 1 "MountingHole_Pad" V 1645 3750 50  0000 L CNN
F 2 "MountingHole:MountingHole_2.7mm_M2.5_Pad" H 1600 3600 50  0001 C CNN
F 3 "~" H 1600 3600 50  0001 C CNN
	1    1600 3600
	0    1    1    0   
$EndComp
$Comp
L power:GND #PWR0111
U 1 1 5F2AC783
P 1400 3700
F 0 "#PWR0111" H 1400 3450 50  0001 C CNN
F 1 "GND" H 1405 3527 50  0000 C CNN
F 2 "" H 1400 3700 50  0001 C CNN
F 3 "" H 1400 3700 50  0001 C CNN
	1    1400 3700
	1    0    0    -1  
$EndComp
Wire Wire Line
	1400 3700 1400 3600
Wire Wire Line
	1400 3000 1500 3000
Wire Wire Line
	1500 3200 1400 3200
Connection ~ 1400 3200
Wire Wire Line
	1400 3200 1400 3000
Wire Wire Line
	1500 3400 1400 3400
Connection ~ 1400 3400
Wire Wire Line
	1400 3400 1400 3200
Wire Wire Line
	1500 3600 1400 3600
Connection ~ 1400 3600
Wire Wire Line
	1400 3600 1400 3400
NoConn ~ 8750 1500
NoConn ~ 9050 1500
NoConn ~ 9250 1500
NoConn ~ 9550 1500
NoConn ~ 9850 1500
Wire Bus Line
	10650 3250 10650 5800
Wire Bus Line
	8450 3250 10650 3250
$EndSCHEMATC
