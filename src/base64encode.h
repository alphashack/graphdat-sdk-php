//
//  base64encode.h
//  graphdat-sdk-php
//
//  Created by Sugendran Ganess on 21/11/12.
//  Copyright (c) 2012 Sugendran Ganess. All rights reserved.
//
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef graphdat_sdk_php_base64encode_h
#define graphdat_sdk_php_base64encode_h

size_t base64_encoded_size(size_t input_length);
void base64_encode(const char *data, size_t input_length, char *encoded_data);

#endif
