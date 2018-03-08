#include "BatchFilter.hpp"
#include "DebugFlags.hpp"
#include "System.hpp"
#include "Observer.hpp"
#include <boost/numeric/odeint.hpp>
#include "FixVectorSize.hpp"

BatchFilter::BatchFilter(const Args & args) : Filter(args){
}

int  BatchFilter::run(
	unsigned int N_iter,
	const arma::vec & X0_true,
	const arma::vec & X_bar_0,
	const std::vector<double> & T_obs,
	const arma::mat & R,
	const arma::mat & Q) {

	
	#if BATCH_DEBUG || FILTER_DEBUG
	std::cout << "- Running filter" << std::endl;
	#endif

	#if BATCH_DEBUG || FILTER_DEBUG
	
	std::cout << "-- Computing true observations" << std::endl;
	#endif


	this -> true_state_history.push_back(X0_true);

	// The true, noisy observations are computed
	this -> compute_true_observations(T_obs,R);

	#if BATCH_DEBUG || FILTER_DEBUG
	std::cout << "-- Done computing true observations" << std::endl;
	#endif


	// Containers
	arma::vec X_bar;
	arma::vec y_bar;


	arma::mat H;

	arma::mat info_mat;
	arma::vec normal_mat;
	arma::vec dx_bar_0 = arma::zeros<arma::vec>(this -> true_state_history[0].n_rows);
	arma::mat P_hat_0;

	bool has_converged;

	// The filter is initialized
	X_bar = X_bar_0;

	this -> info_mat_bar_0 = arma::zeros<arma::mat>(this -> true_state_history[0].n_rows,this -> true_state_history[0].n_rows);
	
	int iterations = N_iter;

	#if BATCH_DEBUG || FILTER_DEBUG
	std::cout << "-- Iterating the filter" << std::endl;
	#endif


	// The batch is iterated
	for (unsigned int i = 0; i <= N_iter; ++i){

		#if BATCH_DEBUG || FILTER_DEBUG
		std::cout << "--- Iteration " << i + 1 << "/" << N_iter << std::endl;
		#endif
		
		#if BATCH_DEBUG || FILTER_DEBUG
		std::cout << "----  Computing prefit residuals" << std::endl;
		#endif

		// The prefit residuals are computed
		this -> compute_prefit_residuals(X_bar,y_bar,has_converged);

		#if BATCH_DEBUG || FILTER_DEBUG
		std::cout << "----  Done computing prefit residuals" << std::endl;
		std::cout << "----  Has converged? " << has_converged << std::endl;
		#endif

		// If the batch was only run for the pass-trough
		if (N_iter == 0){

			try{
				P_hat_0 = arma::inv(this -> info_mat_bar_0);
			}

			catch (std::runtime_error & e){
				P_hat_0.set_size(arma::size(this -> info_mat_bar_0));
				P_hat_0.fill(arma::datum::nan);
			}

			break;
		}

		double N_mes;
		// The state is checked for convergence based on the residuals
		if (has_converged && i != 0){
			iterations = i;
			break;
		}
		else{
			#if BATCH_DEBUG || FILTER_DEBUG

			arma::vec y_non_zero = y_bar.elem(arma::find(y_bar));

			double rms_res = std::sqrt(std::pow(arma::norm(y_non_zero),2) / y_non_zero.n_rows) /T_obs.size();
			N_mes = y_non_zero.n_rows;


			std::cout << "-----  Has not converged" << std::endl;
			std::cout << "-----  Residuals: " << rms_res << std::endl;
			#endif
		}

		// The normal and information matrices are assembled
		info_mat = this -> info_mat_bar_0;
		normal_mat = this ->  info_mat_bar_0 * dx_bar_0;


		#if BATCH_DEBUG || FILTER_DEBUG
		std::cout << "----  Assembling normal equations" << std::endl;
		#endif

		// H has already been pre-multiplied by the corresponding gains
		H = this -> estimate_jacobian_observations_fun(T_obs[0], X_bar ,this -> args);

		// H is divided by the number of effective measurements
		H *= 1./ std::sqrt(N_mes);





		info_mat += H.t() * H;
		normal_mat += H.t() * y_bar;


		// The deviation is solved
		auto dx_hat = arma::solve(info_mat,normal_mat);

		#if BATCH_DEBUG || FILTER_DEBUG
		std::cout << "--- Info mat: \n" << info_mat << std::endl;
		std::cout << "--- Info mat rank: " << arma::rank(info_mat) << std::endl;
		std::cout << "--- Normal mat:\n " << normal_mat << std::endl;
		std::cout << "---  Deviation: "<< std::endl;
		std::cout << dx_hat << std::endl;
		#endif

		// The covariance of the state at the initial time is computed
		P_hat_0 = arma::inv(info_mat);

		// The deviation is applied to the state

		arma::vec X_hat_0 = X_bar + dx_hat;

		X_bar = X_hat_0;


		// The a-priori deviation is adjusted
		dx_bar_0 = dx_bar_0 - dx_hat;

	}

	// The results are saved

	this -> estimated_state_history.push_back(X_bar);
	this -> estimated_covariance_history.push_back(P_hat_0);
	
	this -> residuals.push_back( y_bar);



	#if BATCH_DEBUG || FILTER_DEBUG
	std::cout << "-- Exiting batch "<< std::endl;
	#endif

	return iterations;

}

void BatchFilter::compute_prefit_residuals(
	const arma::vec & X_bar,
	arma::vec & y_bar,
	bool & has_converged){

	// The previous residuals are discarded
	double rms_res = 0;

	
	arma::vec true_obs = this -> true_obs_history[0];
	arma::vec computed_obs = this -> estimate_observation_fun(0, X_bar ,this -> args);

		// Potentials nan are looked for and turned into zeros
		// the corresponding row in H will be set to zero
	arma::vec residual = true_obs - computed_obs;

	#pragma omp parallel for
	for (unsigned int i = 0; i < residual.n_rows; ++i){
		if (residual.subvec(i,i).has_nan() || std::abs(residual(i)) > 1e10 ){
			residual(i) = 0;

		}
	}

	y_bar = residual;

	arma::vec y_non_zero = residual.elem(arma::find(residual));
		#if BATCH_DEBUG
	std::cout << " - Usable residuals in batch: " << y_non_zero.n_rows << std::endl;
		#endif
	rms_res += std::sqrt(std::pow(arma::norm(y_non_zero),2) / y_non_zero.n_rows);

	

	
	has_converged = false;
	

}





