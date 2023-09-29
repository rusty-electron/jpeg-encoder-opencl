#pragma once
#include <cstdint>

const uint8_t JFIF_BYTE_0    = 0x00;
    
/** 
 * JPEG-JFIF File Markers
 * 
 * Refer to ITU-T.81 (09/92), page 32
 */
const uint8_t JFIF_BYTE_FF    = 0xFF; /** All markers start with this as the MSB                  */
const uint8_t JFIF_SOF0       = 0xC0; /** Start of Frame 0, Baseline DCT                          */ 
const uint8_t JFIF_SOF1       = 0xC1; /** Start of Frame 1, Extended Sequential DCT               */
const uint8_t JFIF_SOF2       = 0xC2; /** Start of Frame 2, Progressive DCT                       */
const uint8_t JFIF_SOF3       = 0xC3; /** Start of Frame 3, Lossless (Sequential)                 */
const uint8_t JFIF_DHT        = 0xC4; /** Define Huffman Table                                    */
const uint8_t JFIF_SOF5       = 0xC5; /** Start of Frame 5, Differential Sequential DCT           */
const uint8_t JFIF_SOF6       = 0xC6; /** Start of Frame 6, Differential Progressive DCT          */
const uint8_t JFIF_SOF7       = 0xC7; /** Start of Frame 7, Differential Loessless (Sequential)   */
const uint8_t JFIF_JPG        = 0xC8; /** JPEG Extensions                                         */
const uint8_t JFIF_SOF9       = 0xC9; /** Extended Sequential DCT, Arithmetic Coding              */
const uint8_t JFIF_SOF10      = 0xCA; /** Progressive DCT, Arithmetic Coding                      */
const uint8_t JFIF_SOF11      = 0xCB; /** Lossless (Sequential), Arithmetic Coding                */
const uint8_t JFIF_DAC        = 0xCC; /** Define Arithmetic Coding                                */
const uint8_t JFIF_SOF13      = 0xCD; /** Differential Sequential DCT, Arithmetic Coding          */
const uint8_t JFIF_SOF14      = 0xCE; /** Differential Progressive DCT, Arithmetic Coding         */
const uint8_t JFIF_SOF15      = 0xCF; /** Differential Lossless (Sequential), Arithmetic Coding   */
const uint8_t JFIF_RST0       = 0xD0; /** Restart Marker 0                                        */
const uint8_t JFIF_RST1       = 0xD1; /** Restart Marker 1                                        */
const uint8_t JFIF_RST2       = 0xD2; /** Restart Marker 2                                        */
const uint8_t JFIF_RST3       = 0xD3; /** Restart Marker 3                                        */
const uint8_t JFIF_RST4       = 0xD4; /** Restart Marker 4                                        */
const uint8_t JFIF_RST5       = 0xD5; /** Restart Marker 5                                        */
const uint8_t JFIF_RST6       = 0xD6; /** Restart Marker 6                                        */
const uint8_t JFIF_RST7       = 0xD7; /** Restart Marker 7                                        */
const uint8_t JFIF_SOI        = 0xD8; /** Start of Image                                          */
const uint8_t JFIF_EOI        = 0xD9; /** End of Image                                            */
const uint8_t JFIF_SOS        = 0xDA; /** Start of Scan                                           */
const uint8_t JFIF_DQT        = 0xDB; /** Define Quantization Table                               */
const uint8_t JFIF_DNL        = 0xDC; /** Define Number of Lines                                  */
const uint8_t JFIF_DRI        = 0xDD; /** Define Restart Interval                                 */
const uint8_t JFIF_DHP        = 0xDE; /** Define Hierarchial Progression                          */
const uint8_t JFIF_EXP        = 0xDF; /** Expand Reference Component                              */
const uint8_t JFIF_APP0       = 0xE0; /** Application Segment 0, JPEG-JFIF Image                  */
const uint8_t JFIF_APP1       = 0xE1; /** Application Segment 1                                   */
const uint8_t JFIF_APP2       = 0xE2; /** Application Segment 2                                   */
const uint8_t JFIF_APP3       = 0xE3; /** Application Segment 3                                   */
const uint8_t JFIF_APP4       = 0xE4; /** Application Segment 4                                   */
const uint8_t JFIF_APP5       = 0xE5; /** Application Segment 5                                   */
const uint8_t JFIF_APP6       = 0xE6; /** Application Segment 6                                   */
const uint8_t JFIF_APP7       = 0xE7; /** Application Segment 7                                   */
const uint8_t JFIF_APP8       = 0xE8; /** Application Segment 8                                   */
const uint8_t JFIF_APP9       = 0xE9; /** Application Segment 9                                   */
const uint8_t JFIF_APP10      = 0xEA; /** Application Segment 10                                  */
const uint8_t JFIF_APP11      = 0xEB; /** Application Segment 11                                  */
const uint8_t JFIF_APP12      = 0xEC; /** Application Segment 12                                  */
const uint8_t JFIF_APP13      = 0xED; /** Application Segment 13                                  */
const uint8_t JFIF_APP14      = 0xEE; /** Application Segment 14                                  */
const uint8_t JFIF_APP15      = 0xEF; /** Application Segment 15                                  */
const uint8_t JFIF_JPG0       = 0xF0; /** JPEG Extension 0                                        */
const uint8_t JFIF_JPG1       = 0xF1; /** JPEG Extension 1                                        */
const uint8_t JFIF_JPG2       = 0xF2; /** JPEG Extension 2                                        */
const uint8_t JFIF_JPG3       = 0xF3; /** JPEG Extension 3                                        */
const uint8_t JFIF_JPG4       = 0xF4; /** JPEG Extension 4                                        */
const uint8_t JFIF_JPG5       = 0xF5; /** JPEG Extension 5                                        */
const uint8_t JFIF_JPG6       = 0xF6; /** JPEG Extension 6                                        */
const uint8_t JFIF_JPG7_SOF48 = 0xF7; /** JPEG Extension 7, Lossless JPEG                         */
const uint8_t JFIF_JPG8_LSE   = 0xF8; /** JPEG Extension 8, Lossless JPEG Extension Parameters    */
const uint8_t JFIF_JPG9       = 0xF9; /** JPEG Extension 9                                        */
const uint8_t JFIF_JPG10      = 0xFA; /** JPEG Extension 10                                       */
const uint8_t JFIF_JPG11      = 0xFB; /** JPEG Extension 11                                       */
const uint8_t JFIF_JPG12      = 0xFC; /** JPEG Extension 12                                       */
const uint8_t JFIF_JPG13      = 0xFD; /** JPEG Extension 13                                       */
const uint8_t JFIF_COM        = 0xFE; /** Comment                                                 */


bool saveToJFIFFile( const std::string&, size_t, size_t, std::string);