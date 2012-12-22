/**
 * @file   url.h
 * @author mathslinux <riegamaths@gmail.com>
 * @date   Thu May 24 23:01:23 2012
 *
 * @brief  The Encode and Decode helper is based on
 * code where i download from http://www.geekhideout.com/urlcode.shtml
 * 
 * 
 */

#ifndef LWQQ_URL_H
#define LWQQ_URL_H

/** 
 * NB: be sure to free() the returned string after use
 * 
 * @param str 
 * 
 * @return A url-encoded version of str
 */
std::string url_encode(const char *str);

/** 
 * NB: be sure to free() the returned string after use
 * 
 * @param str 
 * 
 * @return A url-decoded version of str
 */
std::string url_decode(const char *str);
std::string url_whole_encode(const char *str);

//char* to_gbk(const char* utf8);

#endif  /* LWQQ_URL_H */
