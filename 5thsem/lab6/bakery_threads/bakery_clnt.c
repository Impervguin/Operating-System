/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <memory.h> /* for memset */
#include "bakery.h"

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { TIMEOUT_SECONDS, 0 };

enum clnt_stat 
get_number_1(struct BAKERY_CALCULATOR *argp, struct BAKERY_CALCULATOR *clnt_res, CLIENT *clnt)
{
	return (clnt_call(clnt, get_number,
		(xdrproc_t) xdr_BAKERY_CALCULATOR, (caddr_t) argp,
		(xdrproc_t) xdr_BAKERY_CALCULATOR, (caddr_t) clnt_res,
		TIMEOUT));
}

enum clnt_stat 
perform_1(struct BAKERY_CALCULATOR *argp, struct BAKERY_CALCULATOR *clnt_res, CLIENT *clnt)
{
	return (clnt_call(clnt, perform,
		(xdrproc_t) xdr_BAKERY_CALCULATOR, (caddr_t) argp,
		(xdrproc_t) xdr_BAKERY_CALCULATOR, (caddr_t) clnt_res,
		TIMEOUT));
}
