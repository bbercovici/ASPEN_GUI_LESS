#ifndef HEADER_SPFH
#define HEADER_SPFH

#include "PointDescriptor.hpp"

class SPFH : public PointDescriptor{

public:

	SPFH(std::shared_ptr<PointNormal> & query_point,
		std::vector<std::shared_ptr<PointNormal> > & points,
		bool keep_correlations = true,int N_bins = 3);
	
	SPFH();



protected:
};


#endif