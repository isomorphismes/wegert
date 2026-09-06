# Wegert portrait and codomain-phase animation for
#     g(z) = (z - 1) (z - 2) (z - 5)
#
# The core colour calculation below is copied from isomorphisms' wegert.R:
# https://gist.github.com/isomorphisms/5a30e61fb305ee52bcff
# The convenience functions have been placed before plat(), as recommended
# by the gist author in the comments.

# ---- copied Wegert.R convenience functions ----
l <- function(x) x/100 - floor(x/100)
arg <- function(x) Arg(x)*360/2/pi
mod <- function(x) l(Mod(x))
i <- complex(imaginary=1)

plat <- function(space,FUN,cex=2,chroma=45,a=66,b=4,c=3){
 func=FUN(space)
 coleur=arg(func)%%360
 light=a+b*mod(func)+c*l(coleur)
 plot(space, pch=46,cex=cex, col=hcl( h=coleur, c=chroma, l=light ) )
 }
# ---- end copied functions ----

g <- function(z) (z-1)*(z-2)*(z-5)

make.square <- function(limit=5.6, resolution=700L) {
 x <- seq(-limit, limit, length.out=resolution)
 y <- seq(-limit, limit, length.out=resolution)
 outer(x, y, function(real, imaginary) real + i*imaginary)
}

draw.frame <- function(filename, theta, pixels=900L, resolution=700L) {
 Z <- make.square(resolution=resolution)
 g.theta <- function(z) exp(i*theta)*g(z)
 fixed.roots <- c(1,2,5)

 png(filename, width=pixels, height=pixels, res=120, bg="#f5f2eb")
 par(pty="s", mar=c(4,4,4,1), bg="#f5f2eb")
 plat(Z, g.theta, cex=1.1)
 abline(h=0, v=0, col=rgb(1,1,1,.65), lwd=.7)
 points(fixed.roots, pch=21, bg="#f5f2eb", col="#181818", cex=1.0)
 title(
  main=bquote(
   "Codomain phase: "*
   g[theta](z)==e^{i*.(round(theta/pi,2))*pi}*g(z)
  ),
  xlab="Re z",
  ylab="Im z"
 )
 dev.off()
}

# Static frame.
draw.frame(
 "wegert_g.png",
 theta=0,
 pixels=1200L,
 resolution=1000L
)

# Full codomain-phase rotation. This multiplies g(z) by exp(i theta);
# it does not rotate or otherwise change the input z.
dir.create("wegert_g_frames", showWarnings=FALSE)
frame.count <- 120L
for (frame in 0:(frame.count-1L)) {
 theta <- 2*pi*frame/frame.count
 draw.frame(
  sprintf("wegert_g_frames/frame_%03d.png", frame),
  theta=theta,
  pixels=720L,
  resolution=620L
 )
}

# Requires ffmpeg on PATH.
system2(
 "ffmpeg",
 c(
  "-hide_banner", "-loglevel", "error", "-y",
  "-framerate", "20",
  "-i", "wegert_g_frames/frame_%03d.png",
  "-c:v", "libx264", "-preset", "slow", "-crf", "18",
  "-pix_fmt", "yuv420p", "-movflags", "+faststart",
  "wegert_g_codomain_phase.mp4"
 )
)
