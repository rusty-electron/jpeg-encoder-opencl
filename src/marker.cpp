#include <arpa/inet.h> // htons
#include <iostream>
#include <cmath>
#include <bitset>
#include <fstream>

#include "huffman.hpp"
#include "markers.hpp"
#include "quantization.hpp"
#include "utils.hpp"

ppm_t m_image;


bool saveToJFIFFile( const std::string& filename, size_t width, size_t height, std::string m_scanData )
{
    std::ofstream m_outputJPEG( filename, std::ios::out | std::ios::binary );
    
    if ( !m_outputJPEG.is_open() || !m_outputJPEG.good() )
        return false;
    
    m_outputJPEG.unsetf( std::ios::skipws );
    
    m_outputJPEG << JFIF_BYTE_FF << JFIF_SOI;
    
    m_outputJPEG << JFIF_BYTE_FF << JFIF_APP0;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x10;
    m_outputJPEG << (uint8_t)0x4A << (uint8_t)0x46 << (uint8_t)0x49 << (uint8_t)0x46 << (uint8_t)0x00; // 'J', 'F', 'I', 'F', '\0'
    
    m_outputJPEG << (uint8_t)0x01;
    m_outputJPEG << (uint8_t)0x01;
    
    m_outputJPEG << (uint8_t)0x01; // We use DPI for denoting pixel density
        
    // Write the X & Y densities
    // We set a DPI of 72 in both X & Y directions
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x48;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x48;
    
    // Write the thumbnail width & height
    // We don't encode the thumbnail data
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x00;
    
    
    ////////////////////////////////////
    // Write the comment segment
    ////////////////////////////////////
    
    // Write the comment marker
    m_outputJPEG << JFIF_BYTE_FF << JFIF_COM;
    
    //std::string comment = "Encoded with libKPEG (https://github.com/TheIllusionistMirage/libKPEG) - Easy to use baseline JPEG library";
    //std::string comment = "Created with GIMP lal alalala";
    std::string comment = "jpeg-opencl";//", foo barz, etc. etc., blah blah...";
    
    // Write the length of the comment segment
    // NOTE: The length includes the two bytes that denote the length
    m_outputJPEG << (uint8_t)( (uint8_t)( ( comment.length()+2 ) >> 8 ) & (uint8_t)0xFF00 ); // the first 8 MSBs
    m_outputJPEG << (uint8_t)( (uint8_t)( comment.length()+2 ) & (uint8_t)0x00FF ); // the next 8 LSBs
            
    // Write the comment (only ASCII characters allowed)
    m_outputJPEG << comment;
    
    ////////////////////////////////////
    // Write Quantization Tables
    ////////////////////////////////////
    
    /*
     * Quantization table for luminance (Y) 
     */
    
    // Write DQT marker
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DQT;
    
    // Write the length of the DQT segment
    // NOTE: The length includes the two bytes that denote the length
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x43;
    
    // Write quantization table info
    // NOTE: Bits 7-4 denote QT#, bits 3-0 denote QT precision
    // libKPEG supports only 8-bit JPEG images, so bits 7-4 are 0
    m_outputJPEG << (uint8_t)0x00;
    
    // Write the 64 entries of the QT in zig-zag order
    for ( int i = 0; i < 64; ++i )
    {
        auto index = zigzagToCoords(i);
        m_outputJPEG << (uint8_t)M_QT_MAT_LUMA[index.first][index.second];
    }
    
    /**
     * Quantization table for chrominance (Cb & Cr)
     */
    
    // Write DQT marker
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DQT;
    
    // Write the length of the DQT segment
    // NOTE: The length includes the two bytes that denote the length
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x43;
    
    // Write quantization table info
    // NOTE: Bits 7-4 denote QT#, bits 3-0 denote QT precision
    // libKPEG supports only 8-bit JPEG images, so bits 7-4 are 0
    m_outputJPEG << (uint8_t)0x01;
    
    // Write the 64 entries of the QT in zig-zag order
    for ( int i = 0; i < 64; ++i )
    {
        auto index = zigzagToCoords( i );
        m_outputJPEG << (uint8_t)M_QT_MAT_CHROMA[index.first][index.second];
    }
    
    
    ////////////////////////////////////
    // Write start of frame 0 segment
    ////////////////////////////////////
    
    // Write SOF-0 marker identifier
    m_outputJPEG << JFIF_BYTE_FF << JFIF_SOF0;
    
    // Write SOF-0 segment length
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x11; // 8 + 3 * 3
    
    // Write data precision
    m_outputJPEG << (uint8_t)0x08;
    
    // Write image dimensions
    
    // Height
    m_outputJPEG << (u_int8_t)( (uint8_t)( height >> 8 ) & (uint8_t)0xFF00 ); // the first 8 MSBs
    m_outputJPEG << (u_int8_t)( (u_int8_t)height & (uint8_t)0x00FF ); // the next 8 LSBs
    
    
    // Width
    m_outputJPEG << (uint8_t)( (uint8_t)( width >> 8 ) & (uint8_t)0xFF00 ); // the first 8 MSBs
    m_outputJPEG << (uint8_t)( (uint8_t)width & (uint8_t)0x00FF ); // the next 8 LSBs
    
    // Write the number of components
    // NOTE: For now, libKPEG doesn't actually remove the chroma components; they're just set to all 0s
    m_outputJPEG << (uint8_t)0x03;
    
    // Write component info for each of the components (each component takes 3 bytes)
    
    // Luminance (Y)
    m_outputJPEG << (uint8_t)0x01; // Component ID (Y=1, Cb=2, Cr=3)
    m_outputJPEG << (uint8_t)0x11; // Sampling factors (Bits 7-4: Horizontal, Bits 3-0: Vertical)
    m_outputJPEG << (uint8_t)0x00; // Quantization table #
    
    // Chrominance (Cb)
    m_outputJPEG << (uint8_t)0x02; // Component ID (Y=1, Cb=2, Cr=3)
    m_outputJPEG << (uint8_t)0x11; // Sampling factors (Bits 7-4: Horizontal, Bits 3-0: Vertical)
    m_outputJPEG << (uint8_t)0x01; // Quantization table #
    
    // Chrominance (Cr)
    m_outputJPEG << (uint8_t)0x03; // Component ID (Y=1, Cb=2, Cr=3)
    m_outputJPEG << (uint8_t)0x11; // Sampling factors (Bits 7-4: Horizontal, Bits 3-0: Vertical)
    m_outputJPEG << (uint8_t)0x01; // Quantization table #
    
    ////////////////////////////////////
    // Write DHT segments
    ////////////////////////////////////
    
    // Luminance, DC HT
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DHT;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x1F; // DHT segment length (including the length bytes)
    // The symbol count for each symbol from 1-bit length to 16-bit length
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x01 << (uint8_t)0x05 << (uint8_t)0x01
                 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01
                 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00
                 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x01 << (uint8_t)0x02
                 << (uint8_t)0x03 << (uint8_t)0x04 << (uint8_t)0x05 << (uint8_t)0x06 << (uint8_t)0x07
                 << (uint8_t)0x08 << (uint8_t)0x09 << (uint8_t)0x0A << (uint8_t)0x0B;
//                  << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00
//                  << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00
//                  << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00;
    
    // Luminance, AC HT
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DHT;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0xB5; // DHT segment length (including the length bytes)
    m_outputJPEG << (uint8_t)0x10 << (uint8_t)0x00 << (uint8_t)0x02 << (uint8_t)0x01 << (uint8_t)0x03
                 << (uint8_t)0x03 << (uint8_t)0x02 << (uint8_t)0x04 << (uint8_t)0x03 << (uint8_t)0x05
                 << (uint8_t)0x05 << (uint8_t)0x04 << (uint8_t)0x04 << (uint8_t)0x00 << (uint8_t)0x00
                 << (uint8_t)0x01 << (uint8_t)0x7D << (uint8_t)0x01 << (uint8_t)0x02 << (uint8_t)0x03
                 << (uint8_t)0x00 << (uint8_t)0x04 << (uint8_t)0x11 << (uint8_t)0x05 << (uint8_t)0x12
                 << (uint8_t)0x21 << (uint8_t)0x31 << (uint8_t)0x41 << (uint8_t)0x06 << (uint8_t)0x13
                 << (uint8_t)0x51 << (uint8_t)0x61 << (uint8_t)0x07 << (uint8_t)0x22 << (uint8_t)0x71
                 << (uint8_t)0x14 << (uint8_t)0x32 << (uint8_t)0x81 << (uint8_t)0x91 << (uint8_t)0xA1
                 << (uint8_t)0x08 << (uint8_t)0x23 << (uint8_t)0x42 << (uint8_t)0xB1 << (uint8_t)0xC1
                 << (uint8_t)0x15 << (uint8_t)0x52 << (uint8_t)0xD1 << (uint8_t)0xF0 << (uint8_t)0x24
                 << (uint8_t)0x33 << (uint8_t)0x62 << (uint8_t)0x72 << (uint8_t)0x82 << (uint8_t)0x09
                 << (uint8_t)0x0A << (uint8_t)0x16 << (uint8_t)0x17 << (uint8_t)0x18 << (uint8_t)0x19
                 << (uint8_t)0x1A << (uint8_t)0x25 << (uint8_t)0x26 << (uint8_t)0x27 << (uint8_t)0x28
                 << (uint8_t)0x29 << (uint8_t)0x2A << (uint8_t)0x34 << (uint8_t)0x35 << (uint8_t)0x36
                 << (uint8_t)0x37 << (uint8_t)0x38 << (uint8_t)0x39 << (uint8_t)0x3A << (uint8_t)0x43
                 << (uint8_t)0x44 << (uint8_t)0x45 << (uint8_t)0x46 << (uint8_t)0x47 << (uint8_t)0x48
                 << (uint8_t)0x49 << (uint8_t)0x4A << (uint8_t)0x53 << (uint8_t)0x54 << (uint8_t)0x55
                 << (uint8_t)0x56 << (uint8_t)0x57 << (uint8_t)0x58 << (uint8_t)0x59 << (uint8_t)0x5A
                 << (uint8_t)0x63 << (uint8_t)0x64 << (uint8_t)0x65 << (uint8_t)0x66 << (uint8_t)0x67
                 << (uint8_t)0x68 << (uint8_t)0x69 << (uint8_t)0x6A << (uint8_t)0x73 << (uint8_t)0x74
                 << (uint8_t)0x75 << (uint8_t)0x76 << (uint8_t)0x77 << (uint8_t)0x78 << (uint8_t)0x79
                 << (uint8_t)0x7A << (uint8_t)0x83 << (uint8_t)0x84 << (uint8_t)0x85 << (uint8_t)0x86
                 << (uint8_t)0x87 << (uint8_t)0x88 << (uint8_t)0x89 << (uint8_t)0x8A << (uint8_t)0x92 << (uint8_t)0x93
                 << (uint8_t)0x94 << (uint8_t)0x95 << (uint8_t)0x96 << (uint8_t)0x97 << (uint8_t)0x98
                 << (uint8_t)0x99 << (uint8_t)0x9A << (uint8_t)0xA2 << (uint8_t)0xA3 << (uint8_t)0xA4
                 << (uint8_t)0xA5 << (uint8_t)0xA6 << (uint8_t)0xA7 << (uint8_t)0xA8 << (uint8_t)0xA9
                 << (uint8_t)0xAA << (uint8_t)0xB2 << (uint8_t)0xB3 << (uint8_t)0xB4 << (uint8_t)0xB5
                 << (uint8_t)0xB6 << (uint8_t)0xB7 << (uint8_t)0xB8 << (uint8_t)0xB9 << (uint8_t)0xBA
                 << (uint8_t)0xC2 << (uint8_t)0xC3 << (uint8_t)0xC4 << (uint8_t)0xC5 << (uint8_t)0xC6
                 << (uint8_t)0xC7 << (uint8_t)0xC8 << (uint8_t)0xC9 << (uint8_t)0xCA << (uint8_t)0xD2
                 << (uint8_t)0xD3 << (uint8_t)0xD4 << (uint8_t)0xD5 << (uint8_t)0xD6 << (uint8_t)0xD7
                 << (uint8_t)0xD8 << (uint8_t)0xD9 << (uint8_t)0xDA << (uint8_t)0xE1 << (uint8_t)0xE2
                 << (uint8_t)0xE3 << (uint8_t)0xE4 << (uint8_t)0xE5 << (uint8_t)0xE6 << (uint8_t)0xE7
                 << (uint8_t)0xE8 << (uint8_t)0xE9 << (uint8_t)0xEA << (uint8_t)0xF1 << (uint8_t)0xF2
                 << (uint8_t)0xF3 << (uint8_t)0xF4 << (uint8_t)0xF5 << (uint8_t)0xF6 << (uint8_t)0xF7
                 << (uint8_t)0xF8 << (uint8_t)0xF9 << (uint8_t)0xFA;
    
    // Chrominance, DC HT
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DHT;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x1F;
    m_outputJPEG << (uint8_t)0x01 << (uint8_t)0x00 << (uint8_t)0x03 << (uint8_t)0x01 << (uint8_t)0x01
                 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x01
                 << (uint8_t)0x01 << (uint8_t)0x01 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00
                 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x00 << (uint8_t)0x01 << (uint8_t)0x02
                 << (uint8_t)0x03 << (uint8_t)0x04 << (uint8_t)0x05 << (uint8_t)0x06 << (uint8_t)0x07
                 << (uint8_t)0x08 << (uint8_t)0x09 << (uint8_t)0x0A << (uint8_t)0x0B;
    
    // Chrominance, AC HT
    m_outputJPEG << JFIF_BYTE_FF << JFIF_DHT;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0xB5; // DHT segment length (including the length bytes)
    m_outputJPEG << (uint8_t)0x11 << (uint8_t)0x00 << (uint8_t)0x02 << (uint8_t)0x01 << (uint8_t)0x02
                 << (uint8_t)0x04 << (uint8_t)0x04 << (uint8_t)0x03 << (uint8_t)0x04 << (uint8_t)0x07
                 << (uint8_t)0x05 << (uint8_t)0x04 << (uint8_t)0x04 << (uint8_t)0x00 << (uint8_t)0x01
                 << (uint8_t)0x02 << (uint8_t)0x77 << (uint8_t)0x00 << (uint8_t)0x01 << (uint8_t)0x02
                 << (uint8_t)0x03 << (uint8_t)0x11 << (uint8_t)0x04 << (uint8_t)0x05 << (uint8_t)0x21
                 << (uint8_t)0x31 << (uint8_t)0x06 << (uint8_t)0x12 << (uint8_t)0x41 << (uint8_t)0x51
                 << (uint8_t)0x07 << (uint8_t)0x61 << (uint8_t)0x71 << (uint8_t)0x13 << (uint8_t)0x22
                 << (uint8_t)0x32 << (uint8_t)0x81 << (uint8_t)0x08 << (uint8_t)0x14 << (uint8_t)0x42
                 << (uint8_t)0x91 << (uint8_t)0xA1 << (uint8_t)0xB1 << (uint8_t)0xC1 << (uint8_t)0x09
                 << (uint8_t)0x23 << (uint8_t)0x33 << (uint8_t)0x52 << (uint8_t)0xF0 << (uint8_t)0x15
                 << (uint8_t)0x62 << (uint8_t)0x72 << (uint8_t)0xD1 << (uint8_t)0x0A << (uint8_t)0x16
                 << (uint8_t)0x24 << (uint8_t)0x34 << (uint8_t)0xE1 << (uint8_t)0x25 << (uint8_t)0xF1
                 << (uint8_t)0x17 << (uint8_t)0x18 << (uint8_t)0x19 << (uint8_t)0x1A << (uint8_t)0x26
                 << (uint8_t)0x27 << (uint8_t)0x28 << (uint8_t)0x29 << (uint8_t)0x2A << (uint8_t)0x35
                 << (uint8_t)0x36 << (uint8_t)0x37 << (uint8_t)0x38 << (uint8_t)0x39 << (uint8_t)0x3A
                 << (uint8_t)0x43 << (uint8_t)0x44 << (uint8_t)0x45 << (uint8_t)0x46 << (uint8_t)0x47
                 << (uint8_t)0x48 << (uint8_t)0x49 << (uint8_t)0x4A << (uint8_t)0x53 << (uint8_t)0x54
                 << (uint8_t)0x55 << (uint8_t)0x56 << (uint8_t)0x57 << (uint8_t)0x58 << (uint8_t)0x59
                 << (uint8_t)0x5A << (uint8_t)0x63 << (uint8_t)0x64 << (uint8_t)0x65 << (uint8_t)0x66
                 << (uint8_t)0x67 << (uint8_t)0x68 << (uint8_t)0x69 << (uint8_t)0x6A << (uint8_t)0x73
                 << (uint8_t)0x74 << (uint8_t)0x75 << (uint8_t)0x76 << (uint8_t)0x77 << (uint8_t)0x78
                 << (uint8_t)0x79 << (uint8_t)0x7A << (uint8_t)0x82 << (uint8_t)0x83 << (uint8_t)0x84
                 << (uint8_t)0x85 << (uint8_t)0x86 << (uint8_t)0x87 << (uint8_t)0x88 << (uint8_t)0x89 << (uint8_t)0x8A
                 << (uint8_t)0x92 << (uint8_t)0x93 << (uint8_t)0x94 << (uint8_t)0x95 << (uint8_t)0x96
                 << (uint8_t)0x97 << (uint8_t)0x98 << (uint8_t)0x99 << (uint8_t)0x9A << (uint8_t)0xA2
                 << (uint8_t)0xA3 << (uint8_t)0xA4 << (uint8_t)0xA5 << (uint8_t)0xA6 << (uint8_t)0xA7
                 << (uint8_t)0xA8 << (uint8_t)0xA9 << (uint8_t)0xAA << (uint8_t)0xB2 << (uint8_t)0xB3
                 << (uint8_t)0xB4 << (uint8_t)0xB5 << (uint8_t)0xB6 << (uint8_t)0xB7 << (uint8_t)0xB8
                 << (uint8_t)0xB9 << (uint8_t)0xBA << (uint8_t)0xC2 << (uint8_t)0xC3 << (uint8_t)0xC4
                 << (uint8_t)0xC5 << (uint8_t)0xC6 << (uint8_t)0xC7 << (uint8_t)0xC8 << (uint8_t)0xC9
                 << (uint8_t)0xCA << (uint8_t)0xD2 << (uint8_t)0xD3 << (uint8_t)0xD4 << (uint8_t)0xD5
                 << (uint8_t)0xD6 << (uint8_t)0xD7 << (uint8_t)0xD8 << (uint8_t)0xD9 << (uint8_t)0xDA
                 << (uint8_t)0xE2 << (uint8_t)0xE3 << (uint8_t)0xE4 << (uint8_t)0xE5 << (uint8_t)0xE6
                 << (uint8_t)0xE7 << (uint8_t)0xE8 << (uint8_t)0xE9 << (uint8_t)0xEA << (uint8_t)0xF2
                 << (uint8_t)0xF3 << (uint8_t)0xF4 << (uint8_t)0xF5 << (uint8_t)0xF6 << (uint8_t)0xF7
                 << (uint8_t)0xF8 << (uint8_t)0xF9 << (uint8_t)0xFA;
    
    ////////////////////////////////////
    // Write start of scan segment
    ////////////////////////////////////
    m_outputJPEG << JFIF_BYTE_FF << JFIF_SOS;
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x0C; // Length of SOS header
    m_outputJPEG << (uint8_t)0x03; // # of components
    m_outputJPEG << (uint8_t)0x01 << (uint8_t)0x00; // HT info for component #1
    m_outputJPEG << (uint8_t)0x02 << (uint8_t)0x11; // HT info for component #2
    m_outputJPEG << (uint8_t)0x03 << (uint8_t)0x11; // HT info for component #3
    m_outputJPEG << (uint8_t)0x00 << (uint8_t)0x3F << (uint8_t)0x00; // Skip bytes
    
    for ( u_int64_t i = 0; i <= m_scanData.size() - 8; i += 8 )
    {
        std::bitset<8> word( m_scanData.substr( i, 8 ) );
        uint8_t w = (uint8_t)word.to_ulong();
        
        m_outputJPEG << w;
        
        if ( w == JFIF_BYTE_FF )
            m_outputJPEG << (uint8_t)0x00;
    }
    
    ////////////////////////////////////
    // Write end marker
    ////////////////////////////////////
    m_outputJPEG << JFIF_BYTE_FF << JFIF_EOI;
    
    m_outputJPEG.close();    
    return true;
}

/////////////////////////////////////////////////////