void main(void) {
    
    count=GetPeriod(100);
		if(count>0) { // Make sure a signal is detected - otherwise, divide my 0 error.
		    T=count/(F_CPU*100.0);
		    f=1/T;
            
		}