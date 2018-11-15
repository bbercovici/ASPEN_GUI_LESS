#include <BatchAttitude.hpp>
#include <Dynamics.hpp>
#include <Observer.hpp>
#include <System.hpp>

#define BATCH_ATTITUDE_DEBUG 1

BatchAttitude::BatchAttitude(const arma::vec & times,  const std::map<int,arma::mat::fixed<3,3> > & M_pcs){

	for (int k = 0; k < M_pcs.size(); ++k){

		if (k != 0){

			RigidTransform rt;

			rt.t_end = times(k);
			rt.t_start = times(0);

			rt.index_end = k;
			rt.index_start = 0;	

			rt.M = M_pcs.at(k);

			this -> absolute_rigid_transforms.push_back(rt);

		}

		else{

			RigidTransform rt;
			
			rt.t_start = times(0);
			rt.t_end= times(0);

			rt.index_start = 0;
			rt.index_end= 0;

			rt.M = M_pcs.at(0);

			this -> absolute_rigid_transforms.push_back(rt);

		}



	}

}




void BatchAttitude::set_a_priori_state(const arma::vec::fixed<6> & initial_state){
	this -> state_estimate_at_epoch = initial_state;
}

void BatchAttitude::set_inertia_estimate(const arma::mat::fixed<3,3> & inertia){
	this -> inertia_estimate = inertia;
}



arma::vec::fixed<6> BatchAttitude::get_state_estimate_at_epoch() const{
	return this -> state_estimate_at_epoch;
}

arma::mat::fixed<6,6> BatchAttitude::get_state_covariance_at_epoch() const{
	return this -> state_covariance_at_epoch;
}


std::vector<arma::vec::fixed<6> > BatchAttitude::get_attitude_state_history() const{
	return this -> attitude_state_history;
}


std::vector<arma::mat::fixed<6,6> > BatchAttitude::get_attitude_state_covariances_history() const{
	return this -> attitude_state_covariances_history;
}

void BatchAttitude::run(const std::map<int, arma::mat::fixed<6,6> > & R_pcs,
	const std::vector<arma::vec::fixed<3> > & mrps_LN){

	int N_iter = 5;

	std::vector<arma::vec::fixed<6> > state_history;
	std::vector<arma::mat::fixed<6,6> > stms;


	arma::mat info_mat(6,6);
	arma::vec normal_mat(6);
	arma::vec residual_vector = arma::vec(3 * this -> absolute_rigid_transforms.size());
	#if BATCH_ATTITUDE_DEBUG
	std::cout << "In BatchAttitude::run\n";
	#endif


	for (int i = 0; i < N_iter; ++i){

		#if BATCH_ATTITUDE_DEBUG
		std::cout << "\tIteration " << i << "\n";
		std::cout << "\tComputing stms and state history\n";
		#endif


		this -> compute_state_stms( state_history,stms);

		#if BATCH_ATTITUDE_DEBUG
		std::cout << "\tBuilding normal equations\n";
		#endif

		this -> build_normal_equations(info_mat,
			normal_mat,
			residual_vector,
			state_history,
			stms,
			mrps_LN,
			R_pcs);

		#if BATCH_ATTITUDE_DEBUG
		std::cout << "\tResiduals RMS: " << std::sqrt(arma::dot(residual_vector,residual_vector)/residual_vector.size())<< std::endl;
		#endif

		#if BATCH_ATTITUDE_DEBUG
		std::cout << "\tSolving for deviation\n";
		#endif
		try{
			arma::vec::fixed<3> dattitude_state = arma::solve(info_mat,normal_mat);


		#if BATCH_ATTITUDE_DEBUG
			std::cout << "\tApplying deviation\n";
		#endif
			this -> state_estimate_at_epoch.subvec(0,2) = RBK::dcm_to_mrp(
				RBK::mrp_to_dcm(this -> state_estimate_at_epoch.subvec(0,2)) 
				* RBK::mrp_to_dcm(dattitude_state.subvec(0,2)) );

			this -> state_estimate_at_epoch.subvec(3,5) += dattitude_state.subvec(3,5);

			#if BATCH_ATTITUDE_DEBUG
			std::cout << "\tInitial state after update: " << this -> state_estimate_at_epoch.t() << std::endl;
		#endif
		}
		catch(std::runtime_error & e){
			e.what();
		}

	}

	try{
		this -> state_covariance_at_epoch = arma::inv(info_mat);


		for (int k = 0; k < stms.size(); ++k){
			this -> attitude_state_history.push_back(state_history[k]);
			this -> attitude_state_covariances_history.push_back(
				stms[k] 
				* this -> state_covariance_at_epoch 
				* stms[k].t());

		}
	}

	catch(std::runtime_error & e){
		e.what();
	}

}

void BatchAttitude::build_normal_equations(
	arma::mat & info_mat,
	arma::vec & normal_mat,
	arma::vec & residual_vector,
	const std::vector<arma::vec::fixed<6> > & state_history,
	const std::vector<arma::mat::fixed<6,6> > & stms,
	const std::vector<arma::vec::fixed<3> > & mrps_LN,
	const std::map<int, arma::mat::fixed<6,6> > & R_pcs) const{

	residual_vector.fill(0);
	
	info_mat.fill(0);

	info_mat.submat(0,0,2,2) = 1e10 * arma::eye<arma::mat>(3,3);// this way, the initial MRP is frozen

	normal_mat.fill(0);

	arma::mat::fixed<3,6> Htilde = arma::zeros<arma::mat>(3,6);
	Htilde.submat(0,0,2,2) = arma::eye<arma::mat>(3,3);
	arma::mat::fixed<3,6> H;

	arma::mat::fixed<3,3> BN_t0 = RBK::mrp_to_dcm(this -> state_estimate_at_epoch.subvec(0,2));

	for (int k = 0; k < this -> absolute_rigid_transforms.size(); ++ k){

		arma::mat::fixed<3,3> LNk = RBK::mrp_to_dcm(mrps_LN.at(this -> absolute_rigid_transforms.at(k).index_start));
		arma::mat::fixed<3,3> LN_t0 = RBK::mrp_to_dcm(mrps_LN.front());

		arma::mat::fixed<3,3> BN_k_mes = (BN_t0 *LN_t0.t()* this -> absolute_rigid_transforms.at(k).M * LNk);
		arma::mat::fixed<3,3> BN_k_computed = RBK::mrp_to_dcm(state_history[this -> absolute_rigid_transforms.at(k).index_start].subvec(0,2));

		H = Htilde * stms[this -> absolute_rigid_transforms.at(k).index_start];

		arma::mat::fixed<3,3> A = BN_k_computed.t() * BN_t0 * LN_t0.t() * this -> absolute_rigid_transforms.at(k).M;

		arma::vec::fixed<3> e0 = {1,0,0};
		arma::vec::fixed<3> e1 = {0,1,0};
		arma::vec::fixed<3> e2 = {0,0,1};

		arma::mat::fixed<3,3> partial_mat;
		partial_mat.row(0) = - e2.t() * A * RBK::tilde(LNk * e1);
		partial_mat.row(1) = - e0.t() * A * RBK::tilde(LNk * e2);
		partial_mat.row(2) = - e1.t() * A * RBK::tilde(LNk * e0);


		arma::mat::fixed<3,3> R = partial_mat * R_pcs.at(this -> absolute_rigid_transforms.at(k).index_start).submat(3,3,5,5) * partial_mat.t();

		residual_vector.rows(3 * k, 3 * k + 2) = RBK::dcm_to_mrp(BN_k_mes * BN_k_computed.t());

		info_mat += H.t() * arma::inv(R) * H;
		normal_mat += H.t() * arma::inv(R) * residual_vector.rows(3 * k, 3 * k + 2);

	}

	
}


void BatchAttitude::compute_state_stms(std::vector<arma::vec::fixed<6> > & state_history,std::vector<arma::mat::fixed<6,6> > & stms) const{

	state_history.clear();
	stms.clear();

	int N_est = this -> state_estimate_at_epoch.n_rows;

	Args args;
	args.set_inertia_estimate(this -> inertia_estimate);
	System dynamics(args,
		N_est,
		Dynamics::attitude_dxdt_inertial_estimate ,
		Dynamics::attitude_jac_dxdt_inertial_estimate,
		0,
		nullptr);

	arma::vec x(N_est + N_est * N_est);
	x.rows(0,N_est - 1) = this -> state_estimate_at_epoch;
	x.rows(N_est,N_est + N_est * N_est - 1) = arma::vectorise(arma::eye<arma::mat>(N_est,N_est));

	std::vector<arma::vec> augmented_state_history;
	typedef boost::numeric::odeint::runge_kutta_cash_karp54< arma::vec > error_stepper_type;
	auto stepper = boost::numeric::odeint::make_controlled<error_stepper_type>( 1.0e-13 ,1.0e-16 );

	std::vector<double> times;

	for (int i = 0 ; i <  this -> absolute_rigid_transforms. size(); ++i){
		times.push_back( this -> absolute_rigid_transforms. at(i).t_start);
	}

	auto tbegin = times.begin();
	auto tend = times.end();
	boost::numeric::odeint::integrate_times(stepper, dynamics, x, tbegin, tend,1e-10,
		Observer::push_back_attitude_state(augmented_state_history));

	for (int i = 0; i < times.size(); ++i){
		arma::mat::fixed<6,6> stm = arma::reshape(augmented_state_history[i].rows(N_est,N_est + N_est * N_est - 1),N_est,N_est);
		state_history.push_back(augmented_state_history[i]);
		stms.push_back(stm);
	}

}
