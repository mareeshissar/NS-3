set view map;
set xlabel "X"
set ylabel "Y"
set cblabel "SINR (dB)"
unset key
plot "lena-dual-stripe.rem" using ($1):($2):(10*log10($4)) with image
