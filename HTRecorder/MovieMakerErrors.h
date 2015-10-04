#pragma once

#ifndef _MMAKER_ERROR_CODES
#define _MMAKER_ERROR_CODES

#define MMAKER_NO_ERROR							0
#define MMAKER_ERROR_UNKNWON					-501
// Will be raise error when parameter is invalid.
#define MMAKER_ERROR_INVALID_PARAM				-502
// Will be raise error when try start while running.
#define MMAKER_ERROR_ALREADY_STARTED			-503
// The error raised when create file as write mode.
#define MMAKER_ERROR_OPEN_FILE					-504
// The error raised when file write.
#define MMAKER_ERROR_FILE_WRITE					-505
#define MMAKER_ERROR_NOT_STARTED				-506

#endif

