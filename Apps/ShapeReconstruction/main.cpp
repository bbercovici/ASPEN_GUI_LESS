#include <Lidar.hpp>
#include <ShapeModelTri.hpp>
#include <ShapeModelBezier.hpp>
#include <ShapeModelImporter.hpp>
#include <ShapeBuilder.hpp>
#include <Observations.hpp>
#include <Dynamics.hpp>
#include <Observer.hpp>
#include <System.hpp>
#include <ShapeBuilderArguments.hpp>

#include <NavigationFilter.hpp>

#include <chrono>
#include <boost/progress.hpp>
#include <boost/numeric/odeint.hpp>

// Various constants that set up the visibility emulator scenario

// Lidar settings
#define ROW_RESOLUTION 128 // Goldeneye
#define COL_RESOLUTION 128 // Goldeneye
#define ROW_FOV 20 // ?
#define COL_FOV 20 // ?

// Instrument specs
#define FOCAL_LENGTH 1e1 // meters
#define INSTRUMENT_FREQUENCY_SHAPE 0.0016  // frequency at which point clouds are collected for the shape reconstruction phase
#define INSTRUMENT_FREQUENCY_NAV 0.000145 // frequency at which point clouds are collected during the navigation phase
#define SKIP_FACTOR 0.92 // between 0 and 1 . Determines the focal plane fraction that will be kept during the navigation phase (as a fraction of ROW_RESOLUTION)

// Noise
#define LOS_NOISE_SD_BASELINE 1e0
#define LOS_NOISE_FRACTION_MES_TRUTH 0.
// Process noise 
#define PROCESS_NOISE_SIGMA_VEL 1e-10 // velocity
#define PROCESS_NOISE_SIGMA_OMEG 1e-12 // angular velocity

// Times (s)
#define T0 0

// Number of obervation times
#define OBSERVATION_TIMES 1600

// Number of navigation times
#define NAVIGATION_TIMES 80

// Number of points to be retained in the shape fitting
#define POINTS_RETAINED 500000

// Ridge coef (regularization of normal equations)
#define RIDGE_COEF 0e-5

// Number of edges in a-priori
#define N_EDGES 3000

// Shape order
#define SHAPE_DEGREE 2

// Target shape
#define TARGET_SHAPE "itokawa_64_scaled_aligned"

// Spin rate (hours)
#define SPIN_RATE 12.

// Orbit inclination (degrees)
#define INCLINATION 45

// Density (kg/m^3)
#define DENSITY 1900

// Use ICP (false if point cloud is generated from true shape)
#define USE_ICP true

// If true, the state covariance is used to provide an a-priori to the batch
#define USE_PHAT_IN_BATCH false

// Filter iterations
#define N_ITER_SHAPE_FILTER 4

// Whether or not the bundle adjustment should be used
#define USE_BA true

// Number of iterations in bundle adjustment
#define N_ITER_BUNDLE_ADJUSTMENT 30

// Number of iterations in the navigation filter measurement update
#define N_ITER_MES_UPDATE 10

// If true, will exit IEKF if consistency test is satisfied
#define USE_CONSISTENCY_TEST false

///////////////////////////////////////////

int main() {

	arma::arma_rng::set_seed(0);

// Ref frame graph
	FrameGraph frame_graph;
	frame_graph.add_frame("B");
	frame_graph.add_frame("L");
	frame_graph.add_frame("N");
	frame_graph.add_frame("E");

	frame_graph.add_transform("N", "B");
	frame_graph.add_transform("N", "E");
	frame_graph.add_transform("N", "L");


	// Shape model formed with triangles
	ShapeModelTri true_shape_model("B", &frame_graph);

#ifdef __APPLE__
	ShapeModelImporter shape_io_truth(
		"/Users/bbercovici/GDrive/CUBoulder/Research/code/ASPEN_gui_less/resources/shape_models/"+ std::string(TARGET_SHAPE) + ".obj", 1, true);
#elif __linux__
	ShapeModelImporter shape_io_truth(
		"../../../resources/shape_models/" +std::string(TARGET_SHAPE) +".obj", 1 , true);
#else

	throw (std::runtime_error("Neither running on linux or mac os"));
#endif

	shape_io_truth.load_obj_shape_model(&true_shape_model);
	
	true_shape_model.construct_kd_tree_shape();


// Lidar
	Lidar lidar(&frame_graph,
		"L",
		ROW_FOV,
		COL_FOV ,
		ROW_RESOLUTION,
		COL_RESOLUTION,
		FOCAL_LENGTH,
		INSTRUMENT_FREQUENCY_SHAPE,
		LOS_NOISE_SD_BASELINE,
		LOS_NOISE_FRACTION_MES_TRUTH);

	// Integrator extra arguments
	Args args;
	args.set_frame_graph(&frame_graph);
	args.set_true_shape_model(&true_shape_model);
	args.set_mu(arma::datum::G * true_shape_model . get_volume() * DENSITY);
	args.set_mass(true_shape_model . get_volume() * DENSITY);
	args.set_lidar(&lidar);
	args.set_sd_noise(LOS_NOISE_SD_BASELINE);
	args.set_sd_noise_prop(LOS_NOISE_FRACTION_MES_TRUTH);
	args.set_use_P_hat_in_batch(USE_PHAT_IN_BATCH);
	args.set_N_iter_mes_update(N_ITER_MES_UPDATE);
	args.set_use_consistency_test(USE_CONSISTENCY_TEST);
	args.set_skip_factor(SKIP_FACTOR);
	

	// Initial state
	arma::vec X0_augmented = arma::zeros<arma::vec>(12);

	// Position
	arma::vec pos_0 = {1000,500,0};
	X0_augmented.rows(0,2) = pos_0;

	// MRP BN 
	arma::vec mrp_0 = {0.,0.,0.};
	X0_augmented.rows(6,8) = mrp_0;

	// Angular velocity in body frame
	double omega = 2 * arma::datum::pi / (SPIN_RATE * 3600);
	arma::vec omega_0 = {0,0,omega};
	X0_augmented.rows(9,11) = omega_0;

	// Velocity determined from sma
	double a = arma::norm(pos_0);
	double v = sqrt(args.get_mu() * (2 / arma::norm(pos_0) - 1./ a));


	arma::vec vel_0_inertial = {0,std::cos(arma::datum::pi/ 180 * INCLINATION)*v,std::sin(arma::datum::pi/ 180 * INCLINATION)*v};

	X0_augmented.rows(3,5) = vel_0_inertial;

	arma::vec times = arma::linspace<arma::vec>(T0, (OBSERVATION_TIMES - 1) * 1./INSTRUMENT_FREQUENCY_SHAPE,OBSERVATION_TIMES); 
	arma::vec times_dense = arma::linspace<arma::vec>(T0,  times(times.n_rows - 1),  OBSERVATION_TIMES * 10); 

	std::vector<double> T_obs,T_obs_dense;

	for (unsigned int i = 0; i < times.n_rows; ++i){
		T_obs.push_back(times(i));
	}
	for (unsigned int i = 0; i < times_dense.n_rows; ++i){
		T_obs_dense.push_back(times_dense(i));
	}

	// Containers
	std::vector<arma::vec> X_augmented,X_augmented_dense;

	auto N_true = X0_augmented.n_rows;

	// Set active inertia here
	args.set_true_inertia(true_shape_model.get_inertia());

	System dynamics(args,N_true,Dynamics::point_mass_attitude_dxdt_inertial );

	typedef boost::numeric::odeint::runge_kutta_cash_karp54< arma::vec > error_stepper_type;
	auto stepper = boost::numeric::odeint::make_controlled<error_stepper_type>( 1.0e-10 , 1.0e-16 );

	boost::numeric::odeint::integrate_times(stepper, dynamics, X0_augmented, T_obs.begin(), T_obs.end(),1e-3,
		Observer::push_back_augmented_state(X_augmented));

	// The orbit is propagated with a finer timestep for visualization purposes
	boost::numeric::odeint::integrate_times(stepper, dynamics, X0_augmented, T_obs_dense.begin(), 
		T_obs_dense.end(),1e-3,
		Observer::push_back_augmented_state(X_augmented_dense));

	arma::mat X_dense(12,X_augmented_dense.size());
	for (unsigned int i = 0; i < X_augmented_dense.size(); ++i){
		X_dense.col(i) = X_augmented_dense[i];
	}
	X_dense.save("../output/trajectory.txt",arma::raw_ascii);
	


	ShapeBuilderArguments shape_filter_args;
	shape_filter_args.set_los_noise_sd_baseline(LOS_NOISE_SD_BASELINE);
	shape_filter_args.set_points_retained(POINTS_RETAINED);
	shape_filter_args.set_N_iter_shape_filter(N_ITER_SHAPE_FILTER);
	shape_filter_args.set_N_edges(N_EDGES);
	shape_filter_args.set_shape_degree(SHAPE_DEGREE);
	shape_filter_args.set_use_icp(USE_ICP);
	shape_filter_args.set_N_iter_bundle_adjustment(N_ITER_BUNDLE_ADJUSTMENT);
	shape_filter_args.set_use_ba(USE_BA);
	

	ShapeBuilder shape_filter(&frame_graph,&lidar,&true_shape_model,&shape_filter_args);
	shape_filter.run_shape_reconstruction(times,X_augmented,true);
	

	// At this stage, the bezier shape model is NOT aligned with the true shape model
	std::shared_ptr<ShapeModelBezier> estimated_shape_model = shape_filter.get_estimated_shape_model();

	// std::shared_ptr<ShapeModelBezier> estimated_shape_model = std::make_shared<ShapeModelBezier>(
		// ShapeModelBezier(&true_shape_model,"E",&frame_graph,1)
		// );

	estimated_shape_model -> shift_to_barycenter();
	estimated_shape_model -> update_mass_properties();

	estimated_shape_model -> shift_to_barycenter();
	estimated_shape_model -> update_mass_properties();

	std::cout << "\ncenter of mass after shifting: " << estimated_shape_model -> get_center_of_mass().t() << std::endl;
	estimated_shape_model -> align_with_principal_axes();
	estimated_shape_model -> update_mass_properties();
	std::cout << "\ncenter of mass after rotating: " << estimated_shape_model -> get_center_of_mass().t() << std::endl;

	estimated_shape_model -> save_both("../output/shape_model/fit_shape_aligned");
	estimated_shape_model -> construct_kd_tree_shape();
	args.set_estimated_shape_model(estimated_shape_model.get());

	// estimated_shape_model -> compute_volume_sd();

	args.set_estimated_mass(estimated_shape_model -> get_volume() * DENSITY);
	args.set_estimated_inertia(estimated_shape_model -> get_inertia() );

	std::cout << "Estimated inertia:" << std::endl;
	std::cout << estimated_shape_model -> get_inertia() << std::endl;
	std::cout << "True inertia:" << std::endl;
	std::cout << true_shape_model. get_inertia() << std::endl;

	std::cout << "\nEstimated volume: " << estimated_shape_model -> get_volume();
	std::cout << "\nTrue volume: " << true_shape_model.get_volume();
	std::cout << "\nVolume sd: " << estimated_shape_model -> get_volume_sd() << std::endl << std::endl;
	// estimated_shape_model -> compute_cm_cov();

	std::cout << "\nCOM covariance: \n" << estimated_shape_model -> get_cm_cov() << std::endl;


	/**
	END OF SHAPE RECONSTRUCTION FILTER
	*/

	/**
	BEGINNING OF NAVIGATION FILTER
	*/

	// A-priori covariance on spacecraft state and asteroid state.
	arma::vec P0_diag = {
		100,100,100,//position
		1e-5,1e-5,1e-5,//velocity
		1e-3,1e-3,1e-3,// mrp
		1e-19,1e-19,1e-19 // angular velocity
	};

	arma::mat P0 = arma::diagmat(P0_diag);
	
	arma::vec X0_true_augmented = X_augmented.back();
	arma::vec X0_estimated_augmented = X_augmented.back();

	X0_estimated_augmented += arma::diagmat(arma::sqrt(P0_diag)) * arma::randn<arma::vec>(X0_estimated_augmented.n_rows);


	std::cout << "True state: " << std::endl;
	std::cout << X0_true_augmented.t() << std::endl;

	std::cout << "Initial Estimated state: " << std::endl;
	std::cout << X0_estimated_augmented.t() << std::endl;

	std::cout << "Initial Error: " << std::endl;
	std::cout << (X0_true_augmented-X0_estimated_augmented).t() << std::endl;

	arma::vec nav_times(NAVIGATION_TIMES);

	// Times
	std::vector<double> nav_times_vec;
	for (unsigned int i = 0; i < NAVIGATION_TIMES; ++i){
		nav_times(i) = T_obs[T_obs.size() - 1] + double(i)/INSTRUMENT_FREQUENCY_NAV;
		nav_times_vec.push_back( nav_times(i));
	}

	NavigationFilter filter(args);
	arma::mat Q = Dynamics::create_Q(PROCESS_NOISE_SIGMA_VEL,PROCESS_NOISE_SIGMA_OMEG);
	filter.set_gamma_fun(Dynamics::gamma_OD);


	filter.set_observations_fun(
		Observations::obs_pos_mrp_ekf_computed,
		Observations::obs_pos_mrp_ekf_computed_jac,
		Observations::obs_pos_mrp_ekf_lidar);	

	filter.set_estimate_dynamics_fun(
		Dynamics::estimated_point_mass_attitude_dxdt_inertial,
		Dynamics::estimated_point_mass_jac_attitude_dxdt_inertial,
		Dynamics::point_mass_attitude_dxdt_inertial);

	filter.set_initial_information_matrix(arma::inv(P0));

	auto start = std::chrono::system_clock::now();
	int iter = filter.run(1,X0_true_augmented,X0_estimated_augmented,nav_times_vec,arma::ones<arma::mat>(1,1),Q);
	auto end = std::chrono::system_clock::now();

	std::chrono::duration<double> elapsed_seconds = end-start;

	std::cout << " Done running filter " << elapsed_seconds.count() << " s\n";

	filter.write_estimated_state("../output/filter/X_hat.txt");
	filter.write_true_state("../output/filter/X_true.txt");
	filter.write_T_obs(nav_times_vec,"../output/filter/nav_times.txt");
	filter.write_estimated_covariance("../output/filter/covariances.txt");


	return 0;
}












