//#define ARMA_NO_DEBUG
#define ARMA_USE_LAPACK
#define ARMA_USE_BLAS
#include<armadillo>
#include <time.h>
using namespace arma;

int main() {
  // Users need to specify two things:
  // 1/ The text file that contains the data
  arma::Mat<double> yv;
  yv.load("data.txt");
  // 2/ Number of lambdas along the solution path at which we solve the GFL problem
  int len=100;
  // The whole solution path for the GFL will be saved in "GFL_altmin.txt"

  double eps = 0.0001;
  double eps2 = 0.00000001;
  int checkk=0;
  int check=0;
  clock_t t1,t2;
  int ite=10;
  int i,l,j,k,g,it,la,lp;
  vec inside;
  arma_rng::set_seed(1);
  int n = yv.n_rows;
  int p = yv.n_cols;
  int n1=n-1;
  yv=yv.t();
  vec theta = zeros(n);
  mat D(p*n1,p*n,fill::zeros);
  for(i = 0;i<p*n1;i=i+p){
    D(span(i,i+p-1),span(i,i+p-1))=eye(p,p);
    D(span(i,i+p-1),span(i+p,p+i+p-1))=-eye(p,p);
  }  
  int Dnr = D.n_rows;
  vec my = sum(yv,1)/n;
  vec y = vectorise(yv);
  mat tempy(p*n,p,fill::zeros);
  for(i=0;i<p*n;i=i+p){
    tempy(span(i,i+p-1),span(0,p-1))=eye(p,p);
  }
  vec avg = tempy * my;
  mat tD = D.t();
  mat uni = eye(p*n,p*n);
  mat gtD = solve(D.t(),uni,solve_opts::fast);
  vec umax = gtD * (y-avg);
  vec umax2 = umax % umax;
  mat umaxm = reshape(umax2,p,n);
  rowvec rs = sqrt(sum(umaxm));
  
  double lambmax = max(rs);
  vec lambseq = linspace<vec>(0, lambmax,len);
  mat thetas1(n,p*len+p,fill::zeros);
  thetas1.cols(0,p-1) = yv.t();
  mat thetas = yv;
  mat newthetas = thetas;
  mat lag1 = diff(thetas,1,1);
  mat newlag1=lag1;
  mat vv(p,n,fill::ones);
  vv = vv*0.1;
  vv.col(n-1)=zeros(p);
  
  uvec fi = find(zeros(1)==0);
  uvec cfi = fi + n-1;
  fi = join_cols(cfi,fi);
  uvec dfi =fi;
  dfi(1) = 0;
  dfi(0)=-1;
  for(i=1;i<n;i++){
    dfi = dfi +1;
    fi = join_cols(fi,dfi);
  }
  fi(2*n-2)=n-2;
  fi(2*n-1) = n-1;
  uvec index;
  mat matindex(len,n,fill::zeros);
  double oldalpha=1;
  double alpha;
  vec ww=ones(n);
  vec tvec;
  mat temp;
  double nu=0.25;
  double nor;
  vec saveclu;
  uvec allsim = find(zeros(1)==1);
  uvec keepdrop;
  uvec drop;
  vec done;
  uvec inn;
  uvec dind;
  for(i=0;i<len;i++){
    int nn=n;
    
    mat oldtemp = vv;
    mat temperr(p,3*n-1,fill::zeros);
    double lamb = lambseq(i);
    if(lamb==0){ thetas = yv;  }
    else {
      int ku = 0;
      check=0;
      while(check==0 & ku<1){
        for(j=0;j<n-1;j=j+1){
          temperr.col(2*n-1+j) = nu*(newlag1.col(j)-newthetas.col(j) + newthetas.col(j+1));
        }
        temp = vv+temperr.cols(2*n-1,3*n-2);
        alpha = (1+sqrt(1+4*oldalpha*oldalpha))/2;
        mat midstep = temp + (oldalpha-1)/alpha*(temp-oldtemp);
        temperr.cols(2*n-1,3*n-2) = midstep-vv;
        vv = midstep;
        for(j=0;j<n;j=j+1){
          newthetas.col(j) = yv.col(j) -( vv.col(fi(2*j))-vv.col(fi(2*j+1)))/ww(j);
        }
        for(j=0;j<n-1;j=j+1){
          tvec = newthetas.col(j) - newthetas.col(j+1) - vv.col(j)/nu;
          nor = as_scalar(sqrt(tvec.t()*tvec));
          if(nor >lamb/nu){newlag1.col(j) = (1-lamb/nu/nor) * tvec;} else {newlag1.col(j) = zeros(p);}
        }
        
        temperr.cols(0,n-1) = newthetas-thetas;
        temperr.cols(n,2*n-2)=newlag1-lag1;
        
        thetas = newthetas;
        lag1 = newlag1;
        
        oldalpha=alpha;
        oldtemp=temp;
        if(abs(temperr).max()<eps){check = 1;}
      }
    }
    newthetas = 2*thetas - (thetas1.submat(0,i*p,thetas.n_cols-1,i*p+p-1)).t();
    saveclu = zeros(newlag1.n_cols);
    for(j=0;j<newlag1.n_cols;j=j+1){
      if(abs(newlag1.col(j)).max()<eps2){saveclu(j)=1;}
    }
    index = find(saveclu==1);
    uvec tempin=find(zeros(1)==1);
    uvec keepl = find(zeros(1)==1);
    drop = find(zeros(1)==1);
    if(index.n_elem>0){
      dind = diff(index);
      if(dind.n_rows==0){dind = find(zeros(1)==0);}
      if(dind.max()>1){
        uvec sil = find(zeros(1)==0);
        uvec ind2 = join_cols(find(dind>1),sil+index.n_elem-1);
        int start=0;
        // drop = find(zeros(1)==1);
        for(lp=0;lp<ind2.n_elem;lp=lp+1){
          uvec bul=index.subvec(ind2(lp),ind2(lp))+1;
          inn = join_cols(index.subvec(start,ind2(lp)),bul);
          tempin = join_cols(tempin,inn);
          tempin = join_cols(tempin,find(zeros(1)==0)-1);
          int keep = inn(0);
          uvec ridi = find(zeros(1)==0);
          mat mww(inn.n_elem,p,fill::zeros);
          for(int al=0;al<p;al=al+1){
            mww.col(al) = ww(inn);
          }
          keepl = join_cols(keepl,ridi+keep);
          mat tmt =yv.cols(inn);
          yv.col(keep) = sum((tmt.t()%mww)/sum(ww(inn)),0).t();
          thetas.col(keep) = mean(thetas.cols(inn),1);
          newthetas.col(keep) = mean(newthetas.cols(inn),1);
          drop = join_cols(drop, inn.subvec(1,inn.n_elem-1));
          ww(keep) = sum(ww(inn));
          start = ind2(lp)+1;
        }
      } else {
        uvec ind2 = find(zeros(1)==0);
        int start = 0;
        for(lp=0;lp<ind2.n_elem;lp=lp+1){
          uvec bul =index.subvec(index.n_elem-1,index.n_elem-1)+1;
          inn = join_cols(index,bul);
          tempin = join_cols(tempin,inn);
          tempin = join_cols(tempin,find(zeros(1)==0)-1);
          int keep = inn(0);
          uvec ridi = find(zeros(1)==0);
          mat mww(inn.n_elem,p,fill::zeros);
          for(int al=0;al<p;al=al+1){
            mww.col(al) = ww(inn);
          }
          keepl = join_cols(keepl,ridi+keep);
          mat tmt =yv.cols(inn);
          yv.col(keep) = sum((tmt.t()%mww)/sum(ww(inn)),0).t();
          thetas.col(keep) = mean(thetas.cols(inn),1);
          newthetas.col(keep) = mean(newthetas.cols(inn),1);
          
          drop = join_cols(drop, inn.subvec(1,inn.n_elem-1));
          ww(keep) = sum(ww(inn));
          start = ind2(lp)+1;
        }
      }
      done = linspace<vec>(0, yv.n_cols-1,yv.n_cols);
      done(drop)=ones(drop.n_elem)*(-1);
      keepdrop=find(done>(-1));
      yv = yv.cols(keepdrop);
      ww = ww(keepdrop);
      thetas = thetas.cols(keepdrop);
      newthetas = newthetas.cols(keepdrop);
      uvec keepindex = find(saveclu==0);
      uvec tire=find(zeros(1)==0);
      uvec kx=join_cols(keepindex,tire+n-1);
      vv = vv.cols(kx);
      lag1 =  diff(newthetas,1,1);;
      newlag1=lag1;
      n= yv.n_cols;
      fi(0) = vv.n_cols-1;
    }
    thetas1.submat(0,i*p+p,thetas.n_cols-1,i*p+p-1+p) = thetas.t();
    thetas = newthetas;
    lag1 = diff(thetas,1,1);
    newlag1=lag1;
  }
  thetas1.save("GFL_altmin.txt", arma_ascii);
}
