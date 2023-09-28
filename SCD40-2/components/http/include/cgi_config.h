/*
 * cgi_confg.h
 *
 *  Created on: 24 mei 2012
 *      Author: d.kleppe
 */

#ifndef CGI_CONFIG_H_
#define CGI_CONFIG_H_

#include "typedefine.h"

/**
 * cgiConfig_t;
 * table item for cgiConfigTable
 */
typedef struct {
	const char * fieldName; ///< fieldname
	const char *formatstr;	///< formatstring or nr of bytes in case of read/write bytes function
	void *dest;				///< pointer to destination 1
	void *dest2;			///< pointer to destination 2
	result_t (* rxConvertfunc)(void * cgi_configPntr , char *strToParse ); ///< pointer to helper function reading
	uint16_t (* txConvertfunc)(void * cgi_configPntr , char *strToSend ); ///< pointer to helper function writing
}cgiConfig_t;


extern const cgiConfig_t cgiConfigTable[];
void  parseCGIstr( const char *str);
int getCGIresult (char *buf);


#endif /* CGI_CONFIG_H_ */
