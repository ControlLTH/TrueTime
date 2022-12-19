
#define KERNEL_MATLAB
#include "ttkernel.h" 
#include "mex.h"

//#include "sendmsg.cpp"

void mexFunction( int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[] )
{
  int networkNbr=1;
  int nextNode;
  RTnetwork *nwsys;


  // Check number and type of arguments. 
  if ( nrhs != 1 ) {
    TT_MEX_ERROR("NCM: Wrong number of input arguments!\nUsage: NCM(state)");
    return;
  }

  /* get Network system */
  char nwsysbuf[100];
  sprintf(nwsysbuf, "_nwsys_%d", networkNbr);
  mxArray *var = (mxArray*)mexGetVariablePtr("global", nwsysbuf);
  if (var == NULL) {
    printf("_nwsys_%d not found!\n", networkNbr);
    return;
  }
  nwsys = (RTnetwork *)(*((long long *)mxGetPr(var)));


  nextNode = (int)*mxGetPr(prhs[0]);


  /* perform sanity checks */
  if ( nextNode < 0 || nextNode > nwsys->nbrOfNodes ) {
    mexPrintf("NCM: out of range: node %d\n",nextNode);
    return;
  }
    

  /* assign value */
  nwsys->nextNode = nextNode;

  //  mexPrintf("set next node to: %d\n", nextNode);
}
