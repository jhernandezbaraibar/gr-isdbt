/* -*- c++ -*- */
/*  
 * Copyright 2015, 2016, 2017
 *   Federico "Larroca" La Rocca <flarroca@fing.edu.uy>
 *   Pablo Belzarena 
 *   Gabriel Gomez Sena 
 *   Pablo Flores Guridi 
 *   Victor Gonzalez Barbone
 * 
 *   Instituto de Ingenieria Electrica, Facultad de Ingenieria,
 *   Universidad de la Republica, Uruguay.
 *  
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *  
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this software; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 * 
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gnuradio/io_signature.h>
#include "frec_interleaver_impl.h"

namespace gr {
  namespace isdbt {


    const int frec_interleaver_impl::d_data_carriers_mode1 = 96;
    int frec_interleaver::d_total_segments = 1; 


    frec_interleaver::sptr
    frec_interleaver::make(int mode, bool IsFullSeg)
    {
      //Define number of segments in transmission
      if (IsFullSeg){
        d_total_segments = 13;
      }else {
        d_total_segments = 1;
      }
      return gnuradio::get_initial_sptr
        (new frec_interleaver_impl(mode, IsFullSeg));
    }

    /*
     * The private constructor
     */
    frec_interleaver_impl::frec_interleaver_impl(int mode, bool IsFullSeg)
      : gr::sync_block("frec_interleaver",
              gr::io_signature::make(1, 1, sizeof(gr_complex)*d_total_segments*d_data_carriers_mode1*((int)pow(2.0,mode-1))),
              gr::io_signature::make(1, 1, sizeof(gr_complex)*d_total_segments*d_data_carriers_mode1*((int)pow(2.0,mode-1))))
      {
      d_mode = mode; 
      d_carriers_per_segment = d_data_carriers_mode1*((int)pow(2.0,mode-1)); 
      d_noutput = d_total_segments*d_carriers_per_segment;
      d_IsFullSeg = IsFullSeg;
      }

    /*
     * Our virtual destructor.
     */
    frec_interleaver_impl::~frec_interleaver_impl()
    {
    }

    int
    frec_interleaver_impl::work(int noutput_items,
        gr_vector_const_void_star &input_items,
        gr_vector_void_star &output_items)
    {
      const gr_complex *in = (const gr_complex *) input_items[0];
      gr_complex *out = (gr_complex *) output_items[0];

      //TODO: Preguntar: Se borra solo al salir del scope??  
      gr_complex data[d_total_segments][d_carriers_per_segment] = {}; 
      gr_complex data_interleaved[d_total_segments][d_carriers_per_segment] = {};
      gr_complex out_before_rand[d_noutput] = {};
      int k = 0;

      if (!d_IsFullSeg)
      {
        memcpy(&out_before_rand, in, d_noutput*sizeof(gr_complex));
        printf("Voy directo a randomizer\n");
        goto Randomizer;
      }

      //1. Intra-Segment interleaving
      printf("1. Intra-Segment interleaving\n"); 
      k=0; 
      for(int i=0; i<d_total_segments; i++){
        for (int j=0; j<d_carriers_per_segment; j++){
          // First we write the matrix rows-first
          data[i][j] = in[k];
          k++;
        }
      }   
      for (int j=0; j<d_total_segments; j++){
        for(int i=0; i<d_carriers_per_segment; i++){
          // First we write the matrix rows-first
          data_interleaved[d_total_segments -j -1][d_carriers_per_segment - i -1] = data[j][i];
          printf("data_interleaved[%i][%i] = %f \n", i, j, data_interleaved[i][j].real());
        }
      } 
      
      //2. Inter-segment rotation
      k = 0;
      for(int i=0; i<d_total_segments; i++){
        for (int j=0; j<d_carriers_per_segment; j++){
          // First we re-write the data matrix, rows-first
          data_interleaved[i][j] = data[i][(i + j) % d_carriers_per_segment];
          printf("Numero magico %i \n", (i + j) % d_carriers_per_segment);
          printf("data_interleaved[%i][%i] = %f \n", i, j, data_interleaved[i][j].real());
        }
      }
      for(int i=0; i<d_total_segments; i++){
        for (int j=0; j<d_carriers_per_segment; j++){
          // First we write the matrix rows-first
          out_before_rand[k] = data_interleaved[i][j];
          k++;
          //printf("out_before_rand[%i] = %f \n", k, out_before_rand[k].real());
        }
      } 
      printf("2. Inter-segment rotation\n");

      //3. Inter-segment randomizer
Randomizer:
      int index = 0;
      printf("3. Inter-segment randomizer\n");
      switch(d_mode)
      {
        case 1:
        {
          for(int i=0; i<d_noutput; i++)
          {
            index = rand_mode_1[i];
            out[index] = out_before_rand[i];
            //printf("out[%i] = %f \n", index, out[index].real());
          }
          break;
        }
        case 2:
        {
          for(int i=0; i<d_noutput; i++)
          {
            index = rand_mode_2[i];
            out[index] = out_before_rand[i];
          }
          break;
        }
        case 3:
        {
          for(int i=0; i<d_noutput; i++)
          {
            index = rand_mode_3[i];
            out[index] = out_before_rand[i];
          }
          break;
        }
        default:
        {
          printf("Error: Incorrect mode\n");
          break;
        }
      }

      this->consume(0, noutput_items);
      // Tell runtime system how many output items we produced.
      return noutput_items;
    }

  } /* namespace isdbt */
} /* namespace gr */