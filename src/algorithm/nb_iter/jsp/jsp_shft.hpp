/*
 * ******************** jsp_shft.hpp ******************************
 * 
 * description:      job shop SHIFT neighborhood
 * 
 * owner:            Andreas Winkler
 *
 * date:             15.12.1998
 *
 */

#ifndef __cplusplus
#error Must use C++ for these types.
#endif
#ifndef _jsp_shft_h
#define _jsp_shft_h

#include "../neighbor.hpp"
#include "../../../lisa_dt/schedule/js_sched.hpp"
#include "../tabu.hpp"
#include "jsp_api.hpp"

/** Job-Shop SHIFT Neighborhood.
    This is the problem dependent SHIFT neighborhood class for the 
    job shop problem (i.e. these neighborhood shifts an arbitrary operation 
    on an other position on machine).\\
    It is inherited from the class JSHOP_API_Ngbh.

    @author Andreas Winkler
    @version 2.2a0
    @see Lisa_Neighborhood
*/
class JSHOP_shift_Ngbh: public JSHOP_API_Ngbh
     {
                int   pos1, pos2, n;
                Lisa_Vector<int> *JOrd;     
	public:	
                Lisa_Vector<int> *tabu_0;
                Lisa_Vector<int> *tabu_1;
		/** construct OSHOP_API_Ngbh with a start schedule
		    and specified problem datas   */
		JSHOP_shift_Ngbh( Lisa_JsSchedule*, Lisa_JsProblem* );
                ~JSHOP_shift_Ngbh(); 
		/** propose a possible move,
		    parameter is ENUM or RAND for enumerative or random
		    generation of neighbor;\\
		    it propose a shift of an arbitrary operation on an other 
		    position on machine;\\
		    it returns OK or !OK   */
		int   prepare_move(int);
		/// do the proposed move \\ it returns OK or !OK
		int   do_move();
		/// initialize the tabulist with specified length
       int   init_tabulist( unsigned int ); 
		/// search in the tabulist \\ it returns OK or !OK
       int   use_tabulist();
		/// set a tabulist-entry
       int   set_tabulist();
		/// copy tabu-params for an entry
       void  store_tabu_param();
		/// set initial tabu_param datas
       void  clean_tabu_param();
     };

#endif



