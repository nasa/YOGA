#! /bin/bash -u

# Watches LaTeX source files and attempts to typeset if any changes.

trap '/bin/rm -f *.{log,aux,toc,bbl,blg,out} ; exit' INT

ROOT=manual_sections_preview

function build {
  pdflatex -halt-on-error -non-interactive -shell-escape $ROOT
  grep "No file $ROOT.bbl." $ROOT.log && bibtex $ROOT && build
  grep "Rerun to get citations correct" $ROOT.log && bibtex $ROOT && build
  grep "Rerun to get cross" $ROOT.log && build
  grep "run LaTeX again"    $ROOT.log && build
}

function info {
  echo
  echo $0 is now sleeping, It will re-run if a change to the tex files
  echo is detected.
  echo
  echo Press Ctrl-c to exit.
  echo
}

build

if [[ $# -gt 0 ]] ; then
    case "$1" in
	"--open")
	    open $ROOT.pdf
	    exit
	    ;;
	"--once")
      FILE=${ROOT}.pdf
      exitFlag=0
      if [ -f "$FILE" ]; then
        echo "$FILE exists."
      else
        echo "$FILE does not exists."
        exitFlag=-1
      fi
	    exit $exitFlag
	    ;;
    esac
fi

info

while $(sleep 1)
do
  #test $(find *.{tex,bib,cls} -maxdepth 0 -newer $ROOT.log | wc -l) -eq 0 || ( build; info )
  test $(find *.{tex,bib} -maxdepth 0 -newer $ROOT.log | wc -l) -eq 0 || ( build; info )
done
