
set rootdir=`dirname $0`
set rootdir=`cd $rootdir && pwd`  
setenv PYTHONPATH ${PYTHONPATH}:${rootdir}
setenv PATH ${PATH}:${rootdir}/../utils
