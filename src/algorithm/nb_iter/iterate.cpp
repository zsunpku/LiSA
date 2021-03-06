/**
* @author  Andreas Winkler
* @version 3.0
*/

#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <iostream>
#include <iomanip>
#include <fstream>

#include "../../main/global.hpp"
#include "../../misc/int2str.hpp"

#include "iterate.hpp"

using namespace std;

//**************************************************************************

Lisa_Iter::Lisa_Iter(Lisa_ControlParameters *CP){
  abort_algorithm = false;
  
  
  abort_at_bound = -MAXNUMBER;
  if (CP->defined("ABORT_BOUND") == Lisa_ControlParameters::DOUBLE){
    abort_at_bound = CP->get_double("ABORT_BOUND");
  }else{
    G_ExceptionList.lthrow("'ABORT_BOUND' undefined, using default.",Lisa_ExceptionList::WARNING);
  }
  cout << "ABORT_BOUND= " << abort_at_bound << endl;
  
  
  abort_stuck = MAXNUMBER;
  if (CP->defined("NUMB_STUCKS") == Lisa_ControlParameters::LONG){
    abort_stuck = CP->get_long("NUMB_STUCKS");
  }else{
    G_ExceptionList.lthrow("'NUMB_STUCKS' undefined, using default.",Lisa_ExceptionList::WARNING);
  }
  cout << "NUMB_STUCKS= " << abort_stuck << endl;
  
  seed = long( time(0) );
  cout << "seed = " << seed << endl;
}

//**************************************************************************
//**************************************************************************
//**************************************************************************

Lisa_ThresholdAccepting::Lisa_ThresholdAccepting(Lisa_ControlParameters *CP):Lisa_Iter(CP){
  gen_nb = RAND;
  if(CP->defined("TYPE") == Lisa_ControlParameters::STRING){
         if(CP->get_string("TYPE") == "ENUM") gen_nb = ENUM;
    else if(CP->get_string("TYPE") == "RAND") gen_nb = RAND;
    else G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);  
  }else  G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);
  
       if(gen_nb == RAND) cout << "TYPE= RAND" << endl;
  else if(gen_nb == ENUM) cout << "TYPE= ENUM" << endl;
  
  prob0 = 12;
  if ( CP->defined("PROB") == Lisa_ControlParameters::LONG){
    prob0 = CP->get_long( "PROB" );
    if (prob0 < 0 ){
      prob0 = 12;
      G_ExceptionList.lthrow("'PROB' out of range (must be nonnegative), using default.",Lisa_ExceptionList::WARNING);
    }
  }else G_ExceptionList.lthrow("'PROB' undefined, using default.",Lisa_ExceptionList::WARNING);
  
  cout << "PROB= " << prob0 << endl;
  
  
  factor0 = double( prob0 ) / 1000.0;
  cout << "FACTOR0= " << factor0 << endl;
   
  max_stuck = 30;
  if ( CP->defined("MAX_STUCK") == Lisa_ControlParameters::LONG){ 
    max_stuck = CP->get_long( "MAX_STUCK" );
    if ( max_stuck < 1 ){
      max_stuck = 30;
      G_ExceptionList.lthrow("'MAX_STUCK' out of range (must be positive), using default.",Lisa_ExceptionList::WARNING);
    }
  }else G_ExceptionList.lthrow("'MAX_STUCK' undefined, using default.",Lisa_ExceptionList::WARNING);

  cout << "MAX_STUCK= " << max_stuck << endl;
}

//**************************************************************************

void Lisa_ThresholdAccepting::iterate( Lisa_Neighbourhood *NB, int objective_type, 
                                       long steps ){
  
  double  t=0, t_old, t_first=0, decr=0;
  bool    accept=0;
  long    stuck_since, total_stuck;
  TIMETYP best_value, best_step, last_break_value;
  int     test, non_move, max_non_move;

  NB->put_orig_to_best();
  NB->set_objective_type( objective_type );
  NB->set_objective( objective_type,ORIG_SOLUTION );
  best_value = NB->get_objective_value( ORIG_SOLUTION );
  last_break_value = best_value;
  best_step = MAXNUMBER;
  stuck_since = total_stuck = 0;
  max_non_move = steps/10;
  non_move = max_non_move;
  long maxsteps = steps;
  
  //initialize progress meter
  cout << "OBJECTIVE= " << 2*best_value << " Not a real objective, please ignore this line." << endl << endl;
  long steps_per_output_line = 1;
  if( steps >= PROGRESS_INDICATOR_STEPS ) steps_per_output_line = (int) ceil((double)steps/PROGRESS_INDICATOR_STEPS);
  
  
  t = double ( NB->get_objective_value(ORIG_SOLUTION) ) * factor0;
  decr = t / steps;
  t_old = t;
  t_first = t;


 
  for (  ; steps; steps-- ){// iteration loop for SA, II and TA:
      

    t = t - decr;
   
    
    test = NB->prepare_move(gen_nb);
    NB->set_objective( objective_type, ORIG_SOLUTION );
    
    if (!(steps%steps_per_output_line)){
      cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << NB->get_objective_value(ORIG_SOLUTION)
           << "  best= " << best_value
           << "  ready= " << setw(3) <<  (int)  (100. * (maxsteps-steps) / maxsteps) 
           << "%" << endl;
    }
    
    if (test==OK){
      if ( NB->do_move() == OK ){
        
        NB->set_objective( objective_type, WORK_SOLUTION );
        
        // deceide whether to accept new solution:
   
            accept = (   NB->get_objective_value(WORK_SOLUTION)
                       - NB->get_objective_value(ORIG_SOLUTION) < t );
          
            if (++total_stuck>=abort_stuck){
              G_ExceptionList.lthrow("Iteration aborted early because algorithm is stuck for too long. You might want to set other parameters.",Lisa_ExceptionList::WARNING);
              abort_algorithm = true;
            }
          
            if(++stuck_since>=max_stuck){ 
              last_break_value = NB->get_objective_value(ORIG_SOLUTION);
              stuck_since = 0;
              t = t_first;
              //t = t_old;
              decr = t / steps;
            }
   
        
        
        if ( accept ){
          NB->accept_solution();
          
          if (( NB->get_objective_value(WORK_SOLUTION) < best_value )){
            total_stuck = 0;
            best_value = NB->get_objective_value(ORIG_SOLUTION);
            NB->put_orig_to_best();
            
            if ( best_value <= abort_at_bound ){
              G_ExceptionList.lthrow("Iteration aborted early because objective reached lower bound. You might want to set other parameters.",Lisa_ExceptionList::WARNING);
              abort_algorithm = true;
            }
          }
          
          if (( NB->get_objective_value(WORK_SOLUTION) < last_break_value )){
            t_old = t;
            stuck_since = 0;
            last_break_value = NB->get_objective_value(ORIG_SOLUTION);
          }
        }
      } // if do_move == OK 
    } // if test == OK
    
    if ( (test==NO_NGHBOURS) || (abort_algorithm) ) steps = 1;
  } // for ...  
}

//**************************************************************************
//**************************************************************************
//**************************************************************************

Lisa_OldSimulatedAnnealing::Lisa_OldSimulatedAnnealing(Lisa_ControlParameters* CP):Lisa_Iter(CP){
  
  gen_nb = RAND;
  if(CP->defined("TYPE") == Lisa_ControlParameters::STRING){
         if(CP->get_string("TYPE") == "ENUM") gen_nb = ENUM;
    else if(CP->get_string("TYPE") == "RAND") gen_nb = RAND;
    else G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);  
  }else  G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);
  
       if(gen_nb == RAND) cout << "TYPE= RAND" << endl;
  else if(gen_nb == ENUM) cout << "TYPE= ENUM" << endl;
  
  prob0 = 12;
  if ( CP->defined("PROB") == Lisa_ControlParameters::LONG){
    prob0 = CP->get_long( "PROB" );
    if (prob0 < 0 ){
      prob0 = 12;
      G_ExceptionList.lthrow("'PROB' out of range (must be nonnegative), using default.",Lisa_ExceptionList::WARNING);
    }
  }else G_ExceptionList.lthrow("'PROB' undefined, using default.",Lisa_ExceptionList::WARNING);
  
  cout << "PROB= " << prob0 << endl;
  
  
  factor0 = -0.01 / log( double( prob0 ) / 100.0 );
  cout << "FACTOR0= " << factor0 << endl;
   
  max_stuck = 30;
  if ( CP->defined("MAX_STUCK") == Lisa_ControlParameters::LONG){ 
    max_stuck = CP->get_long( "MAX_STUCK" );
    if ( max_stuck < 1 ){
      max_stuck = 30;
      G_ExceptionList.lthrow("'MAX_STUCK' out of range (must be positive), using default.",Lisa_ExceptionList::WARNING);
    }
  }else G_ExceptionList.lthrow("'MAX_STUCK' undefined, using default.",Lisa_ExceptionList::WARNING);

  cout << "MAX_STUCK= " << max_stuck << endl;
  
}
    
void
Lisa_OldSimulatedAnnealing::iterate(Lisa_Neighbourhood *ngbh,
                                    int objective_type, long steps){
  
  double  t=0, t_old, t_first=0, t_end=0, decr=0;
  bool    accept=0;
  long    stuck_since, total_stuck;
  TIMETYP best_value, best_step, last_break_value;
  int     test, non_move, max_non_move;
  
  long maxsteps = steps;
  
 
  
  // getting some memory <-- removed this -marc-
  
  ngbh->put_orig_to_best();
  ngbh->set_objective_type( objective_type );
  ngbh->set_objective( objective_type,ORIG_SOLUTION );
  best_value = ngbh->get_objective_value( ORIG_SOLUTION );
  last_break_value = best_value;
  best_step = MAXNUMBER;
  stuck_since = total_stuck = 0;
  max_non_move = steps/10;
  non_move = max_non_move;
  
  //initialize progress meter
  cout << "OBJECTIVE= " << 2*best_value << " Not a real objective, please ignore this line." << endl << endl;
  long steps_per_output_line = 1;
  if( maxsteps >= PROGRESS_INDICATOR_STEPS ) steps_per_output_line = (int) ceil((double)maxsteps/PROGRESS_INDICATOR_STEPS);
  
  t = double ( ngbh->get_objective_value(ORIG_SOLUTION) ) * factor0;
  t_end = double ( ngbh->get_objective_value(ORIG_SOLUTION) )
  * (-0.001 / log( exp( -3 * log( 10. )) ) );
  
  decr = 1 / exp( log(t/t_end) / steps );
  t_old = t;
  t_first = t;
  
  
  
  for (  ; steps; steps-- ){// iteration loop for SA, II and TA:
    
    
    t = t * decr;
    
    
    test = ngbh->prepare_move(gen_nb);
    ngbh->set_objective( objective_type, ORIG_SOLUTION );
    
    if (!(steps%steps_per_output_line)){
      cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << ngbh->get_objective_value(ORIG_SOLUTION)
           << "  best= " << best_value
           << "  ready= " << setw(3) << 100 - (int)  (100. * (maxsteps-steps) / maxsteps) 
           << "%" << endl;
    }

    
    if (test==OK){
      if ( ngbh->do_move() == OK ){
        
        ngbh->set_objective( objective_type, WORK_SOLUTION );
        
        // deceide whether to accept new solution:
        
        accept = (   ngbh->get_objective_value(WORK_SOLUTION)
        < ngbh->get_objective_value(ORIG_SOLUTION) );
        
        if( !accept ){
          accept = ( lisa_random( 0, 1000000, &seed ) <
          1000000*exp(-(ngbh->get_objective_value(WORK_SOLUTION) - ngbh->get_objective_value(ORIG_SOLUTION))/t));
        }
        
        if (++total_stuck>=abort_stuck){
          G_ExceptionList.lthrow("Iteration aborted early because algorithm is stuck for too long. You might want to set other parameters.",
          Lisa_ExceptionList::WARNING);
          abort_algorithm = true;
        }
        
        if (++stuck_since>=max_stuck){
          
          last_break_value = 2*ngbh->get_objective_value(ORIG_SOLUTION);
          stuck_since = 0;
          t = t_first;
          
          if ( steps > 5 ) decr = 1/exp( log(t/t_end) / steps );
        }

        
        if ( accept ){
          ngbh->accept_solution();
          
          if (( ngbh->get_objective_value(WORK_SOLUTION) < best_value )){
            total_stuck = 0;
            best_value = ngbh->get_objective_value(ORIG_SOLUTION);
            ngbh->put_orig_to_best();
            
            if ( best_value <= abort_at_bound ){
              G_ExceptionList.lthrow("Iteration aborted early because objective reached lower bound. You might want to set other parameters.",Lisa_ExceptionList::WARNING);
              abort_algorithm = true;
            }
          }
          
          if (( ngbh->get_objective_value(WORK_SOLUTION) < last_break_value )){
            t_old = t;
            stuck_since = 0;
            last_break_value = ngbh->get_objective_value(ORIG_SOLUTION);
          }
        }
      } // if do_move == OK 
    } // if test == OK
    
    if ( (test==NO_NGHBOURS) || (abort_algorithm) ) steps = 1;
  } // for ...
}

//**************************************************************************
//**************************************************************************
//**************************************************************************

Lisa_TabuSearch::Lisa_TabuSearch(Lisa_ControlParameters* CP):Lisa_Iter(CP){
  
  gen_nb = RAND;
  if(CP->defined("TYPE") == Lisa_ControlParameters::STRING){
         if(CP->get_string("TYPE") == "ENUM") gen_nb = ENUM;
    else if(CP->get_string("TYPE") == "RAND") gen_nb = RAND;
    else G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);  
  }else  G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);
  
       if(gen_nb == RAND) cout << "TYPE= RAND" << endl;
  else if(gen_nb == ENUM) cout << "TYPE= ENUM" << endl;
  
  
  number_of_neighbours = 40;
  if(CP->defined("NUMB_NGHB") == Lisa_ControlParameters::LONG){
    number_of_neighbours = CP->get_long("NUMB_NGHB");
  }else G_ExceptionList.lthrow("'NUMB_NGHB' undefined, using default.",Lisa_ExceptionList::WARNING);
  
  cout << "NUMB_NGHB= " << number_of_neighbours << endl;
  
  
  tl_length = 40;
  if(CP->defined("TABULENGTH") == Lisa_ControlParameters::LONG){
    tl_length = CP->get_long("TABULENGTH");
  }else G_ExceptionList.lthrow("'TABULENGTH' undefined, using default.",Lisa_ExceptionList::WARNING); 
  
  cout << "TABULENGTH= " << tl_length << endl;
  
}

//**************************************************************************
    
void
Lisa_TabuSearch::iterate(Lisa_Neighbourhood *ngbh,
                         int objective_type, long maxsteps){

  ngbh->put_orig_to_best();
  ngbh->init_tabulist(tl_length);
  ngbh->set_objective_type( objective_type );
  ngbh->set_objective( objective_type,ORIG_SOLUTION );
  TIMETYP best_objective = ngbh->get_objective_value( ORIG_SOLUTION );

  //initialize progress meter
  cout << "OBJECTIVE= " << 2*best_objective << " Not a real objective, please ignore this line." << endl << endl;
  long steps_per_output_line = 1;
  if( maxsteps >= PROGRESS_INDICATOR_STEPS ) steps_per_output_line = (int) ceil((double)maxsteps/PROGRESS_INDICATOR_STEPS);

  long max_non_move = maxsteps/10;
  long non_move= max_non_move;

  long steps = maxsteps;
  
  // iteration loop for tabu search
  while( steps > 0 ){
   
    int nn=number_of_neighbours;
    TIMETYP best_step = MAXTIME;
    ngbh->clean_tabu_param();
    
    while( (nn>0) && (ngbh->prepare_move(gen_nb)==OK)){
      
      ngbh->set_objective( objective_type, ORIG_SOLUTION );
      
      if ( ngbh->use_tabulist() == OK ){
        nn--;
        
        if( ngbh->do_move() == OK ){
          
          steps--;
          
          //write some progress
          if (!(steps%steps_per_output_line)){
             cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << ngbh->get_objective_value(ORIG_SOLUTION)
                  << "  best= " << best_objective 
                  << "  ready= " << setw(3) << 100 - (int)  (100. * steps / maxsteps) 
                  << "%" << endl;
          }
        
          non_move = max_non_move;
          ngbh->set_objective( objective_type, WORK_SOLUTION );
          
          if (ngbh->get_objective_value( WORK_SOLUTION ) < best_step ){
            ngbh->put_work_to_best_ngh();
            ngbh->store_tabu_param();
            best_step=ngbh->get_objective_value( WORK_SOLUTION );
          }
          
        }else{
          
          non_move--;
          
          if ( non_move == 0 ){
            non_move = max_non_move;
            steps--;
            
            //write some progress
           if (!(steps%steps_per_output_line)){
             cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << ngbh->get_objective_value(ORIG_SOLUTION)
                  << "  best= " << best_objective 
                  << "  ready= " << setw(3) << 100 - (int)  (100. * steps / maxsteps) 
                  << "%" << endl;
             }
          }
        }
      }else{
        ngbh->clean_tabu_param();
        ngbh->set_tabulist();
      }
    }
    
    non_move--;
    
    if ( non_move == 0 ){
      non_move = max_non_move;
      steps--;
      
      //write some progress
      if (!(steps%steps_per_output_line)){
        cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << ngbh->get_objective_value(ORIG_SOLUTION)
             << "  best= " << best_objective 
             << "  ready= " << setw(3) << 100 - (int)  (100. * steps / maxsteps) 
             << "%" << endl;
        }
    
    }
    
    ngbh->set_objective( objective_type, ORIG_SOLUTION );
    
    if ( best_step < MAXTIME ) ngbh->accept_best_ngh();
    
    if ( best_step < best_objective ){
      best_objective = ngbh->get_objective_value( ORIG_SOLUTION );
      ngbh->put_orig_to_best();
      
      if ( best_objective <= abort_at_bound ){
        G_ExceptionList.lthrow("Iteration aborted early because objective reached lower bound. You might want to set other parameters.",
        Lisa_ExceptionList::WARNING);
        return;
      }
    }
    
    if(abort_algorithm) return;
    
    ngbh->set_tabulist();
    
  }

}

//**************************************************************************
//**************************************************************************
//**************************************************************************

Lisa_SimulatedAnnealing::Lisa_SimulatedAnnealing(Lisa_ControlParameters* CP):Lisa_Iter(CP){

  if(CP->defined("TSTART") == Lisa_ControlParameters::DOUBLE){
    Tstart = CP->get_double("TSTART");
  }else{
    G_ExceptionList.lthrow("'TSTART' undefined, using default.",Lisa_ExceptionList::WARNING);
    Tstart = 20;
  }
  
  if(CP->defined("TEND") == Lisa_ControlParameters::DOUBLE){
    Tend = CP->get_double("TEND");
  }else{
    G_ExceptionList.lthrow("'TEND' undefined, using default.",Lisa_ExceptionList::WARNING);
    Tend = 0.9;
  }
  
  if(Tstart <= Tend) Tstart = 2*Tend; 
  cout << "TSTART= " << Tstart << endl;
  cout << "TEND= " << Tend << endl;
  
  if(CP->defined("COOLSCHEME") == Lisa_ControlParameters::STRING){
    if(CP->get_string("COOLSCHEME") == "LUNDYANDMEES") cs = LUNDYANDMEES;
    else if(CP->get_string("COOLSCHEME") == "GEOMETRIC") cs = GEOMETRIC;
    else if(CP->get_string("COOLSCHEME") == "LINEAR") cs = LINEAR;
    else{
      G_ExceptionList.lthrow("'COOLSCHEME' undefined, using default.",Lisa_ExceptionList::WARNING);
      cs = LUNDYANDMEES;
    }
  }else{
    G_ExceptionList.lthrow("'COOLSCHEME' undefined, using default.",Lisa_ExceptionList::WARNING);
    cs = LUNDYANDMEES;
  }
  
  if(CP->defined("COOLPARAM") == Lisa_ControlParameters::DOUBLE){
    cp = CP->get_double("COOLPARAM");
  }else{
    G_ExceptionList.lthrow("'COOLPARAM' undefined, using default.",Lisa_ExceptionList::WARNING);
    cp = 0.0005;
  }
  
  switch(cs){
    case GEOMETRIC:
      if(cp > 1) cp = 1;
      if(cp < 0) cp = 0;
      cout << "COOLSCHEME= GEOMETRIC" << endl;
      break;
    case LUNDYANDMEES:
      if(cp < 0) cp = 0;
      cout << "COOLSCHEME= LUNDYANDMEES" << endl;
      break;
    case LINEAR:
      if(cp < 0) cp = 0;
      cout << "COOLSCHEME= LINEAR" << endl;
      break;
  }
  
  cout << "COOLPARAM= " << cp << endl;
  
  if(CP->defined("EPOCH") == Lisa_ControlParameters::LONG){
    epochlength = CP->get_long("EPOCH");
  }else{
    G_ExceptionList.lthrow("'EPOCH' undefined, using default.",Lisa_ExceptionList::WARNING);
    epochlength = 100;
  }
  cout << "EPOCH= " << epochlength << endl;
  
  if(CP->defined("NUMB_NGHB") == Lisa_ControlParameters::LONG){
    numberneighbours = CP->get_long("NUMB_NGHB");
  }else{
    G_ExceptionList.lthrow("'NUMB_NGHB' undefined, using default.",Lisa_ExceptionList::WARNING);
    numberneighbours = 1;
  }
  cout << "NUMB_NGBH= " << numberneighbours << endl;
  
}

//**************************************************************************

void Lisa_SimulatedAnnealing::iterate(Lisa_Neighbourhood *ngbh, 
                                      int objective_type,
                                      long maxsteps){
  ngbh->put_orig_to_best();
  ngbh->set_objective_type(objective_type);
  ngbh->set_objective(objective_type,ORIG_SOLUTION);
  TIMETYP best_objective = ngbh->get_objective_value( ORIG_SOLUTION );
  
  long steps = 0;
  long stuck = 0;

  //initialize progress meter
  cout << "OBJECTIVE= " << 2*best_objective << " Not a real objective, please ignore this line." << endl << endl;
  long steps_per_output = 1;
  if( maxsteps >= PROGRESS_INDICATOR_STEPS ) steps_per_output = (int) ceil((double)maxsteps/PROGRESS_INDICATOR_STEPS);

  double T = Tstart;
  
  while(true){
    
    //do we need to abort ?
    if(steps > maxsteps) return;
    if(stuck > abort_stuck){
      G_ExceptionList.lthrow("Iteration stuck for too long and abortet. You might want to set other parameters.",
                             Lisa_ExceptionList::WARNING);
      return; 
    }
    if(best_objective <= abort_at_bound){
      G_ExceptionList.lthrow("Iteration aborted early because objective reached given lower bound. You might want to set other parameters.",
                             Lisa_ExceptionList::WARNING);
      return;
    }
    if(abort_algorithm){
      return;
    }
    
    //do we need to restart cooling ?
    if(T < Tend) T = Tstart;
    
    
    //generate some neighbours
    for(int i=0;i<numberneighbours;){
      
      TIMETYP best_nb_objective = 0;
      
      int test = ngbh->prepare_move(RAND);
      ngbh->set_objective(objective_type,ORIG_SOLUTION);
      
      if(test == NO_NGHBOURS){
        G_ExceptionList.lthrow("Iteration aborted because Neighbourhood did return NO_NGHBOURS.",
                               Lisa_ExceptionList::WARNING);
        return;      
      }else if(test == OK){
        if(ngbh->do_move() == OK){
          
          ngbh->set_objective(objective_type,WORK_SOLUTION);
          if(i==0 || best_nb_objective > ngbh->get_objective_value(WORK_SOLUTION)){
            ngbh->put_work_to_best_ngh();
            best_nb_objective = ngbh->get_objective_value(BEST_NGH_SOLUTION);
          }
          
          i++;
        }
      }
    }
        
    
    TIMETYP improvement = ngbh->get_objective_value(ORIG_SOLUTION)
                        - ngbh->get_objective_value(BEST_NGH_SOLUTION);
        
    bool accept = (improvement > 0);
    if(!accept) accept = ( lisa_random(0,1000000,&seed ) < 1000000*exp(improvement/T));
        
    if(accept){
      ngbh->accept_best_ngh();
            
      if(ngbh->get_objective_value(ORIG_SOLUTION) < best_objective){
        stuck = 0;
        best_objective = ngbh->get_objective_value(ORIG_SOLUTION);
        ngbh->put_orig_to_best(); 
      }
    }
    
    steps++;
        
    //reduce temperature
    if(!(steps%epochlength)){
      switch(cs){
        case GEOMETRIC:
          T = cp*T;
          break;
        case LUNDYANDMEES:
          T = T/(1+cp*T);
          break;
       case LINEAR:
          T = T - cp*Tstart;
          break;
      }
    }

    //write some progress
    if (!(steps%steps_per_output)){
    cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << ngbh->get_objective_value(ORIG_SOLUTION)
         << "  best= " << best_objective 
         << "  ready= " << setw(3) << (int)  (100. * steps / maxsteps) 
         << "%  temp= " << setprecision(4) << T
          << endl;
    }

    stuck++;
  }
} 
  
//************************************************************************** 
//**************************************************************************  
//**************************************************************************

Lisa_IterativeImprovement::Lisa_IterativeImprovement(Lisa_ControlParameters* CP):Lisa_Iter(CP){
  
  gen_nb = RAND;
  if(CP->defined("TYPE") == Lisa_ControlParameters::STRING){
         if(CP->get_string("TYPE") == "ENUM") gen_nb = ENUM;
    else if(CP->get_string("TYPE") == "RAND") gen_nb = RAND;
    else G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);  
  }else  G_ExceptionList.lthrow("'TYPE' undefined, using default.",Lisa_ExceptionList::WARNING);
  
       if(gen_nb == RAND) cout << "TYPE= RAND" << endl;
  else if(gen_nb == ENUM) cout << "TYPE= ENUM" << endl;

}
  
//**************************************************************************
    
void
Lisa_IterativeImprovement::iterate(Lisa_Neighbourhood *ngbh, int objective_type, long maxsteps){


  ngbh->put_orig_to_best();
  ngbh->init_tabulist( 1 );
  ngbh->set_objective_type( objective_type );
  ngbh->set_objective( objective_type,ORIG_SOLUTION );
  TIMETYP best_objective = ngbh->get_objective_value( ORIG_SOLUTION );
  
  long stuck_since = 0;

  //initialize progress meter
  cout << "OBJECTIVE= " << 2*best_objective << " Not a real objective, please ignore this line." << endl << endl;
  long steps_per_output_line = 1;
  if( maxsteps >= PROGRESS_INDICATOR_STEPS ) steps_per_output_line = (int) ceil((double)maxsteps/PROGRESS_INDICATOR_STEPS);


  for (int steps=0;steps<maxsteps;steps++){
    
    //do we need to abort
    if( stuck_since > abort_stuck ){
      G_ExceptionList.lthrow("Iteration aborted early because algorithm is stuck for too long. You might want to set other parameters",
                              Lisa_ExceptionList::WARNING);
      return;
    }
    if( best_objective <= abort_at_bound ){
      G_ExceptionList.lthrow("Iteration aborted early because objective reached lower bound. You might want to set other parameters.",
                              Lisa_ExceptionList::WARNING);
      return;
    }
    if(abort_algorithm) return; 

    
    int test = ngbh->prepare_move(gen_nb);
    ngbh->set_objective( objective_type, ORIG_SOLUTION );

    
    if (test==OK){
      if ( ngbh->do_move() == OK ){
        
        ngbh->set_objective( objective_type, WORK_SOLUTION );
        TIMETYP new_objective = ngbh->get_objective_value(WORK_SOLUTION);

        if ( new_objective < best_objective ){
          ngbh->accept_solution();
          ngbh->put_orig_to_best();
          best_objective = new_objective;
          stuck_since = 0;          
        }else{
          stuck_since++;  
        }
        
      } 
    }else if(test==NO_NGHBOURS){
      abort_algorithm = true;
    }
    
    if(!(steps%steps_per_output_line)){
      cout << "OBJECTIVE= " << setprecision(0) << setiosflags(ios_base::fixed) << best_objective
           << "  ready= " << setw(3) << (int)  (100. * steps / maxsteps) 
           << "%" << endl;
    }
  }

}
  
//**************************************************************************

