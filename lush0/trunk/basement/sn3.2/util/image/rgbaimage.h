#define BLK_AVRG(nlin, ncol) {\
          register int k,l; int norm = ncol * nlin;\
          register int acc0=0, acc1=0, acc2=0, acc3=0;\
          for (k=0; k<nlin; k++) {\
            register unsigned char *pinptr = pin+k*in_mod0;\
            for (l=0; l<ncol; l++) {\
              acc0 += pinptr[0];acc1 += pinptr[1];acc2 += pinptr[2];acc3 += pinptr[3];\
              pinptr += in_mod1;\
              }}\
            pout[0] = acc0 / norm;pout[1] = acc1 / norm;pout[2] = acc2 / norm; pout[3]= acc3 / norm;}

#define BLK_COPY(nlin, ncol) {\
          register int k,l;\
          register int acc0=pin[0], acc1=pin[1], acc2=pin[2], acc3=pin[3];\
          for (k=0; k<nlin; k++) {\
            register unsigned char *poutptr = pout+k*out_mod0;\
            for (l=0; l<ncol; l++) {\
               poutptr[0]=acc0 ; poutptr[1]=acc1 ; poutptr[2]=acc2 ; poutptr[3]=acc3 ;\
              poutptr += out_mod1;\
                                    }}}
