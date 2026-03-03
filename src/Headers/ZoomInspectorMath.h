#ifndef ZOOMINSPECTORMATH_H
#define ZOOMINSPECTORMATH_H

struct ZoomViewParams {
    double alpha0RadJ2000 = 0.0;
    double dec0RadJ2000 = 0.0;
    double p0Rad = 0.0;
    double beta1Rad = 0.0;
    double beta2Rad = 0.0;
    double pRad = 0.0;
    double fovXRad = 0.0;
    double fovYRad = 0.0;
    double maxMagnitude = 0.0;
    int obsDay = 0;
    int obsMonth = 0;
    int obsYear = 0;
};

namespace zoominspector {

bool computeNewCenterFromClick(
    const ZoomViewParams& view,
    double clickedXi,
    double clickedEta,
    double& outRaJ2000Rad,
    double& outDecJ2000Rad
);

}

#endif // ZOOMINSPECTORMATH_H
