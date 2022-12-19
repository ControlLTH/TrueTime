/* Utility function for checking the input arguments to a MEX function */

#include "mexhelp.h"

bool inline checkinputargs(int nrhs, const mxArray *prhs[], int type0) {
  if (nrhs != 1) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}

bool inline checkinputargs(int nrhs, const mxArray *prhs[], int type0, int type1) {
  if (nrhs != 2) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  default:
    return false;
  }
  switch (type1) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[1])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[1])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}

bool inline checkinputargs(int nrhs, const mxArray *prhs[], int type0, int type1, int type2) {
  if (nrhs != 3) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type1) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[1])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[1])) return false;
    break;
  default:
    return false;
  }
  switch (type2) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[2])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[2])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}

bool inline checkinputargs(int nrhs, const mxArray *prhs[], int type0, int type1, int type2, int type3) {
  if (nrhs != 4) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type1) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[1])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[1])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type2) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[2])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[2])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type3) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[3])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[3])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}

bool inline checkinputargs(int nrhs, const mxArray *prhs[], int type0, int type1, int type2, int type3, int type4) {
  if (nrhs != 5) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type1) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[1])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[1])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type2) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[2])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[2])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type3) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[3])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[3])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type4) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[4])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[4])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}

bool checkinputargs(int nrhs, const mxArray *prhs[], int type0, int type1, int type2, int type3, int type4, int type5) {
  if (nrhs != 6) return false;

  switch (type0) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[0])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[0])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type1) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[1])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[1])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type2) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[2])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[2])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type3) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[3])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[3])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type4) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[4])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[4])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  switch (type5) {
  case TT_SCALAR:
    if (!mxIsDoubleScalar(prhs[4])) return false;
    break;
  case TT_STRING:
    if (!mxIsPlainString(prhs[4])) return false;
    break;
  case TT_STRUCT:
    break;
  default:
    return false;
  }
  return true;
}
