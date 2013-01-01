//
//  syslog.h
//  Firedrake
//
//  Created by Sidney Just
//  Copyright (c) 2013 by Sidney Just
//  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated 
//  documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
//  the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, 
//  and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
//  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, 
//  INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR 
//  PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE 
//  FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
//  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

/**
 * Overview:
 * Defines a simple interface for logging
 **/
#ifndef _SYSLOG_H_
#define _SYSLOG_H_

/**
 * Logging levels where LOG_ALERT is the highest and LOG_DEBUG the lowest
 **/
typedef enum 
{
	LOG_ALERT = 0,
	LOG_CRITICAL,
	LOG_ERROR,
	LOG_WARNING,
	LOG_INFO,
	LOG_DEBUG	
} syslog_level_t;

void syslog(syslog_level_t level, const char *format, ...);

#define dbg(...) syslog(LOG_DEBUG, __VA_ARGS__) // Macro for LOG_DEBUG
#define err(...) syslog(LOG_ERROR, __VA_ARGS__) // Macro for LOG_ERROR
#define warn(...) syslog(LOG_WARNING, __VA_ARGS__) // Macro for LOG_WARNING
#define info(...) syslog(LOG_INFO, __VA_ARGS__) // Macro for LOG_INFO

#endif /* _SYSLOG_H_ */
