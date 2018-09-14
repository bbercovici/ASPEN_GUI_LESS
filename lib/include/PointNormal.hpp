#ifndef HEADER_POINTNORMAL
#define HEADER_POINTNORMAL

#include <armadillo>
#include <memory>
#include "PointFeatureDescriptor.hpp"

class PointNormal {

public:

	PointNormal(arma::vec point);
	PointNormal(arma::vec point, int inclusion_counter) ;
	PointNormal(arma::vec point, arma::vec normal);


	double distance(std::shared_ptr<PointNormal> other_point) const;

	arma::vec get_point() const;

	arma::vec get_normal() const;

	void set_normal(arma::vec normal) ;
	void set_point(arma::vec point) ;
	void set_descriptor(const PointFeatureDescriptor & descriptor) ;
	PointFeatureDescriptor get_descriptor() const;


	void decrement_inclusion_counter();

	int get_inclusion_counter() const;

protected:

	arma::vec point;
	arma::vec normal;

	int inclusion_counter = 0;
	PointFeatureDescriptor descriptor;

};




#endif