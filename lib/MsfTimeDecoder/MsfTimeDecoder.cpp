
#include "MsfTimeDecoder.h"

// Pointer to singleton.

MsfTimeDecoder *MsfTimeDecoderSingleton;

MsfTimeDecoder::MsfTimeDecoder()
{
    // What does this do?
}

void MsfTimeDecoder::init()
{
    // Set up singleton service.

    MsfTimeDecoderSingleton = this;
}
