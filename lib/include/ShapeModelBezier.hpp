#ifndef HEADER_SHAPEMODELBEZIER
#define HEADER_SHAPEMODELBEZIER

#include "ShapeModel.hpp"
#include "Bezier.hpp"

#include "CustomReductions.hpp"

class ShapeModelTri;


class ShapeModelBezier : public ShapeModel{


public:

	/**
	Constructor
	@param shape_model pointer to polyhedral shape model used to construct 
	this new shape model
	@param frame_graph Pointer to the graph storing
	reference frame relationships
	@param frame_graph Pointer to the reference frame graph
	*/
	ShapeModelBezier(ShapeModelTri * shape_model,
		std::string ref_frame_name,
		FrameGraph * frame_graph);

	/**
	Constructor
	@param shape_model pointer to polyhedral shape model used to construct 
	this new shape model
	@param frame_graph Pointer to the graph storing
	reference frame relationships
	@param frame_graph Pointer to the reference frame graph
	@param surface_noise use in patch covariance setting
	*/
	ShapeModelBezier(ShapeModelTri * shape_model,
		std::string ref_frame_name,
		FrameGraph * frame_graph,double surface_noise);

	/**
	Constructor
	@param frame_graph Pointer to the graph storing
	reference frame relationships
	@param frame_graph Pointer to the reference frame graph
	*/
	ShapeModelBezier(std::string ref_frame_name,
		FrameGraph * frame_graph);

	/**
	Constructor.
	Takes a single patch as argument
	@param patch pointer to patch the shape model will be comprised off
	*/
	ShapeModelBezier(Bezier patch);

	std::shared_ptr<arma::mat> get_info_mat_ptr() const;
	std::shared_ptr<arma::vec> get_dX_bar_ptr() const;

	/**
	Constructs a kd tree for ray-tracing purposes
	*/
	virtual void construct_kd_tree_shape();


	/**
	Updates the values of the center of mass, volume
	*/
	virtual void update_mass_properties();


	void initialize_info_mat();
	void initialize_dX_bar();

	arma::mat random_sampling(unsigned int N,const arma::mat & R = 1e-6 * arma::eye<arma::mat>(3,3)) const;


	/**
	Saves the shape model to an obj file as a polyhedron
	*/
	void save_to_obj(std::string path) ;


	/**
	Saves the shape model as a collection of Bezier patches
	*/
	void save(std::string path) ;

	/**
	Elevates the degree of all Bezier patches in the shape model by one
	@param update true if the mass properties/kd tree of the shape model should be updated , false otherwise
	*/
	void elevate_degree(bool update = true);

	/**
	Gets the shape model degree
	*/
	unsigned int get_degree();

	/**
	Computes the surface area of the shape model
	*/
	virtual void compute_surface_area();
	/**
	Computes the volume of the shape model
	*/
	virtual void compute_volume();
	/**
	Computes the center of mass of the shape model
	*/
	virtual void compute_center_of_mass();
	/**
	Computes the inertia tensor of the shape model
	*/
	virtual void compute_inertia();

	/**
	Finds the intersect between the provided ray and the shape model
	@param ray pointer to ray. If a hit is found, the ray's internal is changed to store the range to the hit point
	*/
	virtual bool ray_trace(Ray * ray);

	void save_both(std::string partial_path);


	/**
	Assembles the tables used to compute the mass properties of the shape
	*/
	void populate_mass_properties_coefs();


	/**
	Compute the standard deviation in the volume
	*/
	void compute_volume_sd();

	/**
	Compute the covariance in the center of mass
	*/
	void compute_cm_cov();

	/**
	Runs a Monte Carlo on volume, cm
	@param N number of runs
	@return results
	*/
	void run_monte_carlo(int N,
		arma::vec & results_volume,
		arma::mat & results_cm,
		arma::mat & results_inertia,
		arma::mat & results_moments,
		arma::mat & results_mrp,
		arma::mat & results_lambda_I,
		arma::mat & results_eigenvectors,
		arma::mat & results_Evectors,
		arma::mat & results_Y,
		arma::mat & results_MI);

	/**
	Runs a Monte Carlo on volume
	@param N number of runs
	@return results
	*/
	arma::mat run_monte_carlo_cm(int N);





	/**
	Return the standard deviation in the volume
	@return standard deviation in shape volume
	*/
	double get_volume_sd() const;


	/**
	Return the center of mass covariance 
	@return center of mass covariance 
	*/
	arma::mat get_cm_cov() const;

	/**
	Return the covariance of the parametrization of the inertia tensor
	@return inertia tensor parametrization covariance 
	*/
	arma::mat get_inertia_cov() const {return this -> P_I;}


	arma::mat get_mrp_cov() const{return this -> P_sigma;}


	void build_structure() ;

	void compute_inertia_statistics() ;


	arma::mat get_P_moments() const{return this -> P_moments;}
	arma::mat get_P_Y() const{return this -> P_Y;}

	arma::rowvec::fixed<6> get_P_lambda_I(const int lambda_index) const ;
	arma::vec::fixed<6> get_P_MI() const{return this -> P_MI;}


	arma::mat::fixed<9,9> get_P_eigenvectors() const{return this -> P_eigenvectors;}
	arma::mat::fixed<9,9> get_P_Evectors() const{return this -> P_Evectors;}
	arma::vec::fixed<9> get_E_vectors() const;

	void apply_deviation();

	arma::vec d_I() const;



protected:








	/**
	Generates all the possible combinations of indices as needed in the inertia moments
	computation from the tensorization of the base vector of Bezier indices
	@param n_indices number of indices to tensorize 
	@parma base_vector vector of indices to tensorize (contains N such indices pairs)
	@param index_vectors container holding the final N ^ n_indices vectors of indices pairs
	@param temp_vector container holding the current vector of indices being built
	@param depth depth of the current vector
	*/
	static void build_bezier_index_vectors(const int & n_indices,
		const std::vector<std::vector<int> > & base_vector,
		std::vector<std::vector<std::vector<int> > > & index_vectors,
		std::vector < std::vector<int> > temp_vector = std::vector < std::vector<int> >(),
		const int depth = 0);


	/**	
	Builds the vector holding the pair of indices of each point in a Bezier triangle
	@param n Bezier triangle degree
	@param base_vector container to hold the pair of indices of each point in a Bezier triangle
	*/
	static void build_bezier_base_index_vector(const int n,std::vector<std::vector<int> > & base_vector);



	void compute_P_I();
	void compute_P_MI();
	void compute_P_Y();
	void compute_P_MX();

	void compute_P_moments();
	void compute_P_sigma();
	void compute_P_eigenvectors();
	void compute_P_Evectors();

	static arma::rowvec::fixed<15> L_row(int q, int r, const arma::vec * Ci,const arma::vec * Cj,const arma::vec * Ck,const arma::vec * Cl,const arma::vec * Cm);



	arma::mat::fixed<3,3> increment_cm_cov(arma::mat::fixed<12,3> & left_mat,
		arma::mat::fixed<12,3>  & right_mat, 
		int i,int j,int k,int l, 
		int m, int p, int q, int r);


	arma::mat::fixed<6,6> increment_P_I(const arma::mat::fixed<6,15> & left_mat,
		const arma::mat::fixed<6,15>  & right_mat, 
		int i,int j,int k,int l,int m,
		int p, int q, int r, int s, int t) const;


	arma::vec::fixed<6> increment_P_MI(arma::mat::fixed<6,15> & left_mat,
		arma::vec::fixed<9>  & right_vec, 
		int i,int j,int k,int l,int m,
		int p, int q, int r);

	void construct_cm_mapping_mat(arma::mat::fixed<12,3> & mat,
		int i,int j,int k,int l);


	void construct_inertia_mapping_mat(arma::mat::fixed<6,15> & mat,
		int i,int j,int k,int l,int m) const;

	static arma::rowvec::fixed<6> partial_T_partial_I() ;
	static arma::rowvec::fixed<4> partial_Theta_partial_W(const double & T,const double & Pi,const double & U,const double & d);
	static double partial_theta_partial_Theta(const double & Theta);
	static arma::rowvec::fixed<3> partial_A_partial_Y(const double & theta,const double & U);
	static arma::rowvec::fixed<3> partial_B_partial_Y(const double & theta,const double & U);
	static arma::rowvec::fixed<3> partial_C_partial_Y(const double & theta,const double & U);
	static arma::mat::fixed<3,7> partial_r_i_partial_I_lambda(const int i);


	arma::mat::fixed<2,6> partial_Z_partial_I() const ;
	arma::rowvec::fixed<6> partial_Pi_partial_I() const ;
	arma::mat::fixed<4,6> partial_W_partial_I() const;
	arma::rowvec::fixed<6> partial_theta_partial_I() const;
	arma::rowvec::fixed<6> partial_U_partial_I() const ;
	arma::rowvec::fixed<2> partial_U_partial_Z() const;
	arma::rowvec::fixed<6> partial_d_partial_I() const ;
	arma::mat::fixed<3,3> partial_elambda_Elambda(const double & lambda) const;
	arma::mat::fixed<9,9> P_E_lambda_E_mu() const ;

	arma::mat::fixed<3,3> P_XX() const ;
	arma::mat::fixed<3,6> partial_X_partial_I() const;
	arma::mat::fixed<4,4> partial_M_partial_Y() const;
	arma::mat::fixed<3,3> get_principal_axes_stable() const;


	arma::mat::fixed<3,3> P_ril_rjm(
		const double lambda, 
		const double mu,
		const int i,
		const int j,
		const int lambda_index,
		const int mu_index,
		const double theta,
		const double U) const;
	arma::mat::fixed<9,9> P_R_lambda_R_mu(const double lambda, const double mu,const int lambda_index,
		const int mu_index,
		const double theta,
		const double U) const ;

	arma::mat::fixed<3,9> partial_E_partial_R(const double lambda) const;

	arma::vec::fixed<4> get_Y() const;


	arma::rowvec::fixed<6> P_lambda_I(const int lambda_index,const double theta, const double U) const;
	arma::mat::fixed<3,6> partial_Y_partial_I() const;

	std::shared_ptr<arma::mat> info_mat_ptr = nullptr;
	std::shared_ptr<arma::vec> dX_bar_ptr = nullptr;
	std::vector<std::vector<double> > cm_gamma_indices_coefs_table;
	std::vector<std::vector<double> > volume_indices_coefs_table;
	std::vector<std::vector<double> > inertia_indices_coefs_table;



	std::vector<std::vector<double> > volume_sd_indices_coefs_table;
	std::vector<std::vector<double> > cm_cov_1_indices_coefs_table;
	std::vector<std::vector<double> > cm_cov_2_indices_coefs_table;

	std::vector<std::vector<double> > inertia_stats_1_indices_coefs_table;
	std::vector<std::vector<double> > inertia_stats_2_indices_coefs_table;




	double volume_sd;
	arma::mat cm_cov = arma::zeros<arma::mat>(3,3);

	arma::mat::fixed<6,6> P_I;
	arma::vec::fixed<6> P_MI;
	arma::mat::fixed<4,4> P_Y;
	arma::vec::fixed<3> P_MX;

	arma::mat::fixed<4,4> P_moments;
	arma::mat::fixed<9,9> P_eigenvectors;
	arma::mat::fixed<9,9> P_Evectors;
	arma::mat::fixed<3,3> P_sigma;



};













#endif