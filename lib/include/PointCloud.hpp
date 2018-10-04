#ifndef HEADER_POINT_CLOUD
#define HEADER_POINT_CLOUD

#include <armadillo>
#include <memory>
#include <cassert>

template <template<class> class ContainerType, class PointType>  class KDTree ;


typedef typename std::pair<int, int > PointPair ;

template <class PointType> 
class PointCloud {

public:

	PointCloud();
	PointCloud(int size);
	PointCloud(const std::vector<PointType> & points);
	PointCloud(std::vector< std::shared_ptr< PointCloud < PointType> > > & pcs,int points_retained);
	PointCloud(std::string filename);

	/**
	Returns the index to point cloud element whose point is closest to the provided test_point
	using the KD Tree search
	@param test_point 3-by-1 vector queried
	@return Pointer to closest point
	*/
	int get_closest_point(const arma::vec & test_point) const;

	/**
	Returns queried point
	@param index Index of the queried point
	@return queried point
	*/
	const PointType & get_point(unsigned int index) const;

	/**
	Returns queried point
	@param index Index of the queried point
	@return queried point
	*/
	PointType & get_point(unsigned int index) ;


	/**
	Returns size of point cloud
	@return size of point cloud
	*/
	unsigned int size() const;

	/**
	Applies rigid transform to point cloud
	@param dcm dcm directing the rotational component of the rigid transform the rigid transform
	@param x translational component of the rigid transform
	*/
	void transform(const arma::mat::fixed<3,3> & dcm = arma::eye<arma::mat>(3,3),const arma::vec::fixed<3> & x = arma::zeros<arma::vec>(3));
	
	/**
	Returns a map to the closest N PointType-s whose coordinates are closest to the provided test_point
	using the KD Tree search
	@param test_point 3-by-1 vector queried
	@return map to these closest points sorted by distance
	*/
	std::map<double,int > get_closest_N_points(const arma::vec & test_point, const unsigned int & N) const;

	/**
	Get label identifying the point cloud
	*/
	std::string get_label() const;

	/**
	Returns a constant reference to the coordinates of the queried point at the provided index
	@param index
	@return vector of coordinates
	*/
	const arma::vec & get_point_coordinates(int i) const;

	/**
	Returns a constant reference to the coordinates of the queried point's normal at the provided index
	Only defined for PointCloud<PointNormal>
	@param index
	@return vector of coordinates
	*/
	const arma::vec & get_normal_coordinates(int i) const;

	/**
	Returns the points of $this that are within the sphere of specified radius centered at $test_point
	@param test_point query point
	@param radius non-negative search radius
	@return vector of indices of points in $this that satisfy || test_point - point_in_$this ||  < radius
	*/
	std::vector<int> get_nearest_neighbors_radius(const arma::vec & test_point, const double & radius) const;


	/**
	Adds an element to the point cloud
	*/
	void push_back(const PointType & point);

	/**
	Builds KD Tree
	*/
	void build_kdtree();

	/**
	Subscript operator accessing the underlying point vector
	*/
	PointType & operator[] (const int index);

	/**
	Checks if indexed point has been deemed value. 
	@param i point index
	@return validity. If the point cloud type is PointCloud<PointNormal>, validity == true always
	*/
	bool check_if_point_valid(int i) const;





protected:

	std::vector<PointType> points;
	std::shared_ptr< KDTree< PointCloud, PointType> > kdt;
	arma::vec mean_feature_histogram;
	std::string label;


};


#endif