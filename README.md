# yas
yas - Yet Another SCOS

Richard Heathfield, from the Usenet Group sci.crpyt, introduced  
his SCOS (sci.crypt open secret) a while ago to the community.  

This is an enhanced version of SCOS, using base64 and allowing
binary input.

The cool thing about yas is, that now you don't know if it is
a standard base64 encoded message or not. :-)

Usage: $ yas e 0 63 < infile > outfile.
       $ yas d 0 63 < infile > outfile
       
Allowed numbers must be between 0 and 63.

