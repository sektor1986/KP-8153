/************************************************************************/
/*               (C) Fujitsu Microelectronics Europe GmbH               */
/*                                                                      */
/* The following software deliverable is intended for and must only be  */
/* used in an evaluation laboratory environment.                        */
/* It is provided without charge and therefore provided on an as-is     */
/* basis, subject to alterations.                                       */
/*                                                                      */
/* The software deliverable is to be used exclusively in connection     */
/* with FME products.                                                   */
/* In the event the software deliverable includes the use of open       */
/* source components, the provisions of the governing open source       */
/* license agreement shall apply with respect to such software          */
/* deliverable.                                                         */
/* FME does not warrant that the deliverables does not infringe any     */
/* third party intellectual property right (IPR). In the event that     */
/* the deliverables infringe a third party IPR it is the sole           */
/* responsibility of the customer to obtain necessary licenses to       */
/* continue the usage of the deliverable.                               */
/*                                                                      */
/* To the maximum extent permitted by applicable law FME disclaims all  */
/* warranties, whether express or implied, in particular, but not       */
/* limited to, warranties of merchantability and fitness for a          */
/* particular purpose for which the deliverable is not designated.      */
/*                                                                      */
/* To the maximum extent permitted by applicable law, FME’s liability   */
/* is restricted to intention and gross negligence.                     */
/* FME is not liable for consequential damages.                         */
/*                                                                      */
/* (V1.1)                                                               */
/************************************************************************/
/*------------------------------------------------------------------------
  zpd_settings.h

  05.05.09  1.00   JWa    Initial version
-------------------------------------------------------------------------*/

#ifndef __ZPD_H__
#define __ZPD_H__

// ----------------------------------------------------------------------------

// Define maximum supported SMC units on that device (starting from 0 ... SMC_ZPD_MAX_STEPPER_UNITS - 1)
#define SMC_ZPD_MAX_STEPPER_UNITS		2

// ----------------------------------------------------------------------------

// min./max. blinding time of one full step (no adc sampling)
// STEP_BLINDING_TIME_US_MAX + STEP_SAMPLING_TIME_US_MAX <= 0x3FFFC!!
#define STEP_BLINDING_TIME_US_MIN		300
#define STEP_BLINDING_TIME_US_DEFAULT	1000
#define STEP_BLINDING_TIME_US_MAX		10000

// min./max. sampling time of one full step
// STEP_BLINDING_TIME_US_MAX + STEP_SAMPLING_TIME_US_MAX <= 0x3FFFC!!
#define STEP_SAMPLING_TIME_US_MIN		3000
#define STEP_SAMPLING_TIME_US_DEFAULT	14000
#define STEP_SAMPLING_TIME_US_MAX		400000

// ----------------------------------------------------------------------------

// Resistor circuit applied to motor coils
// Circuit A = internal pull-up resistor on SMC-P, internal pull-down resistor on SMC-M
// Circuit B = internal pull-up resistor on SMC-P, external pull-down resistor on SMC-P
// Note: for 16FX devices RESISTOR_CIRCUIT_A is not possible
#define RESISTOR_CIRCUIT_A				0xA
#define RESISTOR_CIRCUIT_B				0xB

#define RESISTOR_CIRCUIT				RESISTOR_CIRCUIT_B	// <<< set resistor circuit

// ----------------------------------------------------------------------------

// ADC sampling time settings
#define ADC_SAMPLING_TIME_4_CLKP1		0
#define ADC_SAMPLING_TIME_6_CLKP1		1
#define ADC_SAMPLING_TIME_8_CLKP1		2
#define ADC_SAMPLING_TIME_12_CLKP1		3
#define ADC_SAMPLING_TIME_24_CLKP1		4
#define ADC_SAMPLING_TIME_36_CLKP1		5
#define ADC_SAMPLING_TIME_48_CLKP1		6
#define ADC_SAMPLING_TIME_128_CLKP1		7

#define ADC_SAMPLING_TIME_SETTING		ADC_SAMPLING_TIME_128_CLKP1		// <<< set ADC sampling time

// ADC comparison time settings
#define ADC_COMPARISON_TIME_22_CLKP1	0
#define ADC_COMPARISON_TIME_33_CLKP1	1
#define ADC_COMPARISON_TIME_44_CLKP1	2
#define ADC_COMPARISON_TIME_66_CLKP1	3
#define ADC_COMPARISON_TIME_88_CLKP1	4
#define ADC_COMPARISON_TIME_132_CLKP1	5
#define ADC_COMPARISON_TIME_176_CLKP1	6
#define ADC_COMPARISON_TIME_264_CLKP1	7

#define ADC_COMPARISON_TIME_SETTING		ADC_COMPARISON_TIME_66_CLKP1	// <<< set ADC comparison time

// ----------------------------------------------------------------------------

// min./max. ADC sammple count (< 0xFFFF)
#define ADC_SAMPLE_COUNT_MIN			5
#define ADC_SAMPLE_COUNT_DEFAULT		75
#define ADC_SAMPLE_COUNT_MAX			250

// ADC sampling intervall: must be greater than sampling_time + comparison_time + 4 µs!
// ATTENTION: This value sets the time between two AD conversions, indepent from the number
//            of stepper motor units that are processed simultaneously!
//            The sampling intervall of one SMC channel is always SMC_ZPD_MAX_STEPPER_UNITS * ADC_SAMPLING_INTERVALL_US
#define ADC_SAMPLING_INTERVALL_US		50							// <<< set ADC sampling intervall time in µs

// ----------------------------------------------------------------------------

// Enable/Disable floating averaging of sampled ADC waveform
#define ADC_FLOATING_AVERAGING			0				// <<< enable (= 1) / disable (= 1) floating averaging
// Definition of ADC floating averaging in 2^x
#define ADC_FLOATING_AVERAGING_POW		2				// <<< set = 4	

// ----------------------------------------------------------------------------

// Calibration settings (measure idle DC voltage of voltage divider)
#define ADC_CALIBRATION_BLIND_LENGTH	10									// = nr. of pre-samples to ignore (not included in ADC_CALIBRATION_LENGTH)
#define ADC_CALIBRATION_POW_2			4									// = nr. of samples (2^x)

// ----------------------------------------------------------------------------

// Form factor algorithms
#define ADC_FORM_FACTOR_ALGORITHM_MINMAX				0	// form_factor = max_value - min_value
#define ADC_FORM_FACTOR_ALGORITHM_EQUAL_DIFF			1	// form_factor = min(rising_part_max, falling_part_max)
#define ADC_FORM_FACTOR_ALGORITHM_EQUAL_INTEGRATE		2	// form_factor = min(pos_area, neg_area) | pos_area = values > ADC_Zero_Line
#define ADC_FORM_FACTOR_ALGORITHM_INTEGRATE		        3	// form_factor = pos_area + neg_area | pos_area = values > ADC_Zero_Line

// Set evaluation algorithm for BEMF profile
#define ADC_FORM_FACTOR_ALGORITHM			ADC_FORM_FACTOR_ALGORITHM_INTEGRATE	// <<< set calculation of form factor

// ----------------------------------------------------------------------------

#endif	// __ZPD_H__
