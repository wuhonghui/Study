/*
 * =====================================================================================
 *
 *       Filename:  kmp_test.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/11/2013 01:02:16 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *        Company:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "kmp.h"

int main(int argc, char *argv[])
{
    assert(0 == kmp("abc", "abcdddd"));
    assert(9 == kmp("ui88", "aui8xxxuiui88"));
    return 0;
}
