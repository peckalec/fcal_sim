// G4 headers
#include "G4Poisson.hh"
#include "Randomize.hh"

#include <CCDB/Calibration.h>
#include <CCDB/Model/Assignment.h>
#include <CCDB/CalibrationGenerator.h>
using namespace ccdb;

// gemc headers
#include "fcal_hitprocess.h"

// CLHEP units
#include "CLHEP/Units/PhysicalConstants.h"
using namespace CLHEP;

static fcalConstants initializeFCALConstants(int runno, string digiVariation = "default", string digiSnapshotTime = "no", bool accountForHardwareStatus = false) {

  fcalConstants ftc;
		
	// do not initialize at the beginning, only after the end of the first event,
	// with the proper run number coming from options or run table
	if (runno == -1) return ftc;
	string timestamp = "";
	if(digiSnapshotTime != "no") {
		timestamp = ":"+digiSnapshotTime;
	}
	
	ftc.runNo = runno;
	ftc.date = "2015-11-29";
	if (getenv("CCDB_CONNECTION") != NULL)
		ftc.connection = (string) getenv("CCDB_CONNECTION");
	else
		ftc.connection = "mysql://clas12reader@clasdb.jlab.org/clas12";
	
		
	ftc.dEdxMIP = 1.956; // muons in polyvinyltoluene
	ftc.pmtPEYld = 500;
	//	ftc.tdcLSB        = 42.5532;// counts per ns (23.5 ps LSB)
	

	for(int layer=0; layer<21; layer++){
          ftc.npaddles[layer] = 40;
          ftc.thick[layer] = 2;
	  ftc.dEMIP[layer] = ftc.thick[layer] * ftc.dEdxMIP;
	}

	double attlen = 200;//dummy value
	double veff = 16;
	for (int lay =0; lay<21; lay++){
	  ftc.tres[0][lay].resize(ftc.npaddles[lay]);
	  for(int paddle=0; paddle<ftc.npaddles[lay]; paddle++){
	    ftc.attlen[0][lay][0].push_back(attlen);
	    ftc.attlen[0][lay][1].push_back(attlen);
	    ftc.veff[0][lay][0].push_back(veff);
            ftc.veff[0][lay][1].push_back(veff);

	    ftc.countsForMIP[0][lay][0].push_back(1000);             
	    ftc.countsForMIP[0][lay][1].push_back(1000);

	    ftc.status[0][lay][0].push_back(0);
	    ftc.status[0][lay][0].push_back(0);

	    ftc.twlk[0][lay][0].push_back(40);
	    ftc.twlk[0][lay][1].push_back(0.5);                     
	    ftc.twlk[0][lay][2].push_back(0);
	    ftc.twlk[0][lay][3].push_back(40);
	    ftc.twlk[0][lay][4].push_back(0.5);                    
	    ftc.twlk[0][lay][5].push_back(0);

	    ftc.toff_LR[0][lay].push_back(0);
	    ftc.toff_RFpad[0][lay].push_back(0);
	    ftc.toff_P2P[0][lay].push_back(0);

	    ftc.tdcconv[0][lay][0].push_back(0.02345 );
	    ftc.tdcconv[0][lay][1].push_back(0.02345 );
	    
	    ftc.tres[0][lay][paddle]=0.1; //dummy value 
	  }
	  
	}
	
	// setting voltage signal parameters
        ftc.vpar[0] = 50; // delay, ns
        ftc.vpar[1] = 10; // rise time, ns
        ftc.vpar[2] = 20; // fall time, ns
        ftc.vpar[3] = 1; // amplifier
	// FOR now we will initialize pedestals and sigmas to a random value, in the future
        // they will be initialized from DB
	
        const double const_ped_value = 101;
        const double const_ped_sigm_value = 2;
        // commands below fill all the elements of ctc.pedestal and ctc.pedestal_sigm with their values (const_ped_value, and const_ped_sigm_value respectively)
        std::fill(&ftc.pedestal[0][0][0][0], &ftc.pedestal[0][0][0][0] + sizeof (ftc.pedestal) / sizeof (ftc.pedestal[0][0][0][0]), const_ped_value);
	
        std::fill(&ftc.pedestal_sigm[0][0][0][0], &ftc.pedestal_sigm[0][0][0][0] + sizeof (ftc.pedestal_sigm) / sizeof (ftc.pedestal_sigm[0][0][0][0]), const_ped_sigm_value);

	// filling translation table                                                         
	for (int lay =0; lay<21; lay++){
	  for(int paddle=0; paddle<ftc.npaddles[lay]; paddle++){
	    for(int pmt=0; pmt<4; pmt++){
	      
	      int crate = lay*210+paddle*10+pmt;//dummy values
	      int slot = lay*210+paddle*10+pmt;
	      int channel = lay*210+paddle*10+pmt;
	      
	      int sector = 0;
	      
	      
	    // order is important as we could have duplicate entries w/o it
	      ftc.TT.addHardwareItem({sector, lay, paddle, pmt}, Hardware(crate, slot, channel));
	    }
	  }
        }
        cout << "  > Data loaded in translation table " << ftc.TT.getName() << endl;
        
	
	
	int isec, ilay;
	
	vector<vector<double> > data;
	/*
	unique_ptr<Calibration> calib(CalibrationGenerator::CreateCalibration(ftc.connection));
	cout << "Connecting to " << ftc.connection << "/calibration/fcal" << endl;
	
	cout << "FCAL:Getting attenuation" << endl;
	sprintf(ftc.database, "/calibration/fcal/attenuation:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.attlen[isec - 1][ilay - 1][0].push_back(data[row][3]);
		ftc.attlen[isec - 1][ilay - 1][1].push_back(data[row][4]);
	}
	
	cout << "FCAL:Getting effective_velocity" << endl;
	sprintf(ftc.database, "/calibration/fcal/effective_velocity:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.veff[isec - 1][ilay - 1][0].push_back(data[row][3]);
		ftc.veff[isec - 1][ilay - 1][1].push_back(data[row][4]);
	}
	
	if(accountForHardwareStatus) {
		cout << "FCAL:Getting status" << endl;
		sprintf(ftc.database, "/calibration/fcal/status:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
		data.clear();
		calib->GetCalib(data, ftc.database);
		for (unsigned row = 0; row < data.size(); row++) {
			isec = data[row][0];
			ilay = data[row][1];
			ftc.status[isec - 1][ilay - 1][0].push_back(data[row][3]);
			ftc.status[isec - 1][ilay - 1][1].push_back(data[row][4]);
		}
	}
	cout << "FCAL:Getting gain_balance" << endl;
	sprintf(ftc.database, "/calibration/fcal/gain_balance:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.countsForMIP[isec - 1][ilay - 1][0].push_back(data[row][3]);
		ftc.countsForMIP[isec - 1][ilay - 1][1].push_back(data[row][4]);
	}
	
	cout << "FCAL:Getting time_walk" << endl;
	sprintf(ftc.database, "/calibration/fcal/time_walk:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.twlk[isec - 1][ilay - 1][0].push_back(data[row][3]);
		ftc.twlk[isec - 1][ilay - 1][1].push_back(data[row][4]);
		ftc.twlk[isec - 1][ilay - 1][2].push_back(data[row][5]);
		ftc.twlk[isec - 1][ilay - 1][3].push_back(data[row][6]);
		ftc.twlk[isec - 1][ilay - 1][4].push_back(data[row][7]);
		ftc.twlk[isec - 1][ilay - 1][5].push_back(data[row][8]);
	}

	cout << "FCAL:Getting time_offset" << endl;
	
	sprintf(ftc.database,"/calibration/fcal/time_offsets:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.toff_LR[isec - 1][ilay - 1].push_back(data[row][3]);
		ftc.toff_RFpad[isec-1][ilay-1].push_back(data[row][4]);
		ftc.toff_P2P[isec-1][ilay-1].push_back(data[row][5]);
		
		// cout << " Loading constant: " << isec << " " << ilay << " " << data[row][5] << " " << digiVariation << endl;
	}
	
	cout << "FCAL:Getting tdc_conv" << endl;
	sprintf(ftc.database, "/calibration/fcal/tdc_conv:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0];
		ilay = data[row][1];
		ftc.tdcconv[isec - 1][ilay - 1][0].push_back(data[row][3]);
		ftc.tdcconv[isec - 1][ilay - 1][1].push_back(data[row][4]);
	}
	
	cout << "FCAL:Getting resolutions" << endl;
	sprintf(ftc.database, "/calibration/fcal/tres:%d:%s%s", ftc.runNo, digiVariation.c_str(), timestamp.c_str());
	data.clear();
	calib->GetCalib(data, ftc.database);
	
	for(isec = 0; isec < 6; isec++) {
		for(ilay = 0; ilay < 3; ilay++) {
			ftc.tres[isec][ilay].resize(ftc.npaddles[ilay]);
		}
	}
	
	for (unsigned row = 0; row < data.size(); row++) {
		isec = data[row][0] - 1;
		ilay = data[row][1] - 1;
		int ipaddle   = data[row][2] - 1;
		double sigma = data[row][3];
		ftc.tres[isec][ilay][ipaddle] = sigma;
	}
	
	// updated on 1/9/2020: leaving this one here for reference. We now use constants from CCDB
	//	cout << "FCAL:Setting time resolution" << endl;
	//	for (int p = 0; p < 3; p++) {
	//		for (int c = 1; c < ftc.npaddles[p] + 1; c++) {
	//			if (p == 0) ftc.tres[p].push_back(1e-3 * (c * 4.545 + 95.465)); //ps to ns
	//			if (p == 1) ftc.tres[p].push_back(1e-3 * (c * 0.820 + 59.16)); //ps to ns
	//			if (p == 2) ftc.tres[p].push_back(1e-3 * (200.0)); //ps to ns fixed number
	//		}
	//	}
	
	
	// setting voltage signal parameters
	ftc.vpar[0] = 50; // delay, ns
	ftc.vpar[1] = 10; // rise time, ns
	ftc.vpar[2] = 20; // fall time, ns
	ftc.vpar[3] = 1; // amplifier
	
	
	// FOR now we will initialize pedestals and sigmas to a random value, in the future
	// they will be initialized from DB
	const double const_ped_value = 101;
	const double const_ped_sigm_value = 2;
	// commands below fill all the elements of ctc.pedestal and ctc.pedestal_sigm with their values (const_ped_value, and const_ped_sigm_value respectively)
	std::fill(&ftc.pedestal[0][0][0][0], &ftc.pedestal[0][0][0][0] + sizeof (ftc.pedestal) / sizeof (ftc.pedestal[0][0][0][0]), const_ped_value);
	std::fill(&ftc.pedestal_sigm[0][0][0][0], &ftc.pedestal_sigm[0][0][0][0] + sizeof (ftc.pedestal_sigm) / sizeof (ftc.pedestal_sigm[0][0][0][0]), const_ped_sigm_value);
	
	string database = "/daq/tt/fcal:1";
	
	data.clear();
	calib->GetCalib(data, database);
	cout << "  > " << ftc.TT.getName() << " TT Data loaded from CCDB with " << data.size() << " columns." << endl;
	
	// filling translation table
	for (unsigned row = 0; row < data.size(); row++) {
		int crate = data[row][0];
		int slot = data[row][1];
		int channel = data[row][2];
		
		int sector = data[row][3];
		int panel = data[row][4];
		int paddle = data[row][5];
		int pmt = data[row][6];
		
		// order is important as we could have duplicate entries w/o it
		ftc.TT.addHardwareItem({sector, panel, paddle, pmt}, Hardware(crate, slot, channel));
		
	}
	cout << "  > Data loaded in translation table " << ftc.TT.getName() << endl;
	*/
	return ftc;
	
}



map<string, double> fcal_HitProcess::integrateDgt(MHit* aHit, int hitn) {
	map<string, double> dgtz;
	
	// hit ids
	vector<identifier> identity = aHit->GetId();
	
	int sector = identity[0].id;
	int panel  = identity[1].id; // 1-1A, 2-1B, 3-2B
	int paddle = identity[2].id;
	int pmt    = identity[3].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide
	
	// TDC conversion factors
	double tdcconv = ftc.tdcconv[sector - 1][panel - 1][pmt][paddle - 1];

	if(aHit->isBackgroundHit == 1) {
		
		// background hit has all the energy in the first step. Time is also first step
		double totEdep = aHit->GetEdep()[0];
		double stepTime = aHit->GetTime()[0];
		double adc = totEdep * ftc.countsForMIP[sector - 1][panel - 1][pmt][paddle - 1] / ftc.dEMIP[panel - 1] ; // no gain as that comes from data already
		double tdc = stepTime/tdcconv;
		
		dgtz["hitn"]      = hitn;
		dgtz["sector"]    = sector;
		dgtz["layer"]     = panel;
		dgtz["component"] = paddle;
		dgtz["ADC_order"] = pmt;
		dgtz["ADC_ADC"]   = (int) adc;
		dgtz["ADC_time"]  = (tdc*24.0/1000);
		dgtz["ADC_ped"]   = 0;

		dgtz["TDC_order"] = pmt + 2;
		dgtz["TDC_TDC"]   = (int) tdc;

		return dgtz;
	}
	
	
	trueInfos tInfos(aHit);
	
	// Get the paddle half-length
	double length = aHit->GetDetector().dimensions[0];
	
	// Distances from left, right
	//	double dLeft  = length + tInfos.lx;
	//	double dRight = length - tInfos.lx;
	
	double d = length + (1 - 2 * pmt) * tInfos.lx;
	
	// attenuation length
	//	double attlenL = ftc.attlen[sector-1][panel-1][0][paddle-1];
	//	double attlenR = ftc.attlen[sector-1][panel-1][1][paddle-1];
	
	double attlen = ftc.attlen[sector - 1][panel - 1][pmt][paddle - 1];
	double attlen_otherside = ftc.attlen[sector - 1][panel - 1][1 - pmt].at(paddle - 1);
	// attenuation factor
	//	double attLeft  = exp(-dLeft/cm/attlenL);
	//	double attRight = exp(-dRight/cm/attlenR);
	
	double att = exp(-d / cm / attlen);
	
	// Gain factors to simulate FCAL PMT gain matching algorithm.
	// Each L,R PMT pair has HV adjusted so geometeric mean sqrt(L*R)
	// is independent of counter length, which compensates for
	// the factor exp(-L/2/attlen) where L=full length of bar.
	//	double gainLeft  = sqrt(attLeft*attRight);
	//	double gainRight = gainLeft;
	
	double gain = sqrt(exp(-d / cm / attlen) * exp(-(2 * length - d) / cm / attlen_otherside));
	
	// Attenuated light at PMT
	//	double eneL = tInfos.eTot*attLeft;
	//	double eneR = tInfos.eTot*attRight;
	
	double ene = tInfos.eTot*att;
	
	// giving geantinos some energies
	if (aHit->GetPID() == 0) {
		double gmomentum = aHit->GetMom().mag() / GeV;
		//		eneL = gmomentum*attLeft;
		//		eneR = gmomentum*attRight;
		
		ene = gmomentum*att;
		
	}
	
	double adc = 0;
	double tdc = 0;
	// not used anymore
//	double adcu = 0;
//	double tdcu = 0;
	
//	if (ene > 0) {
//		adcu = ene * ftc.countsForMIP[sector - 1][panel - 1][pmt][paddle - 1] / ftc.dEMIP[panel - 1] / gain;
//	}

	// Fluctuate the light measured by the PMT with
	// Poisson distribution for emitted photoelectrons
	// Treat L and R separately, in case nphe=0
	
		
	
	double nphe = G4Poisson(ene * ftc.pmtPEYld);
	ene = nphe / ftc.pmtPEYld;
	
	if (ene > 0) {
		adc = ene * ftc.countsForMIP[sector - 1][panel - 1][pmt][paddle - 1] / ftc.dEMIP[panel - 1] / gain;
		double A = ftc.twlk[sector - 1][panel - 1][3 * pmt + 0][paddle - 1];
		double B = ftc.twlk[sector - 1][panel - 1][3 * pmt + 1][paddle - 1];
		//double            C = ftc.twlk[sector-1][panel-1][2][paddle-1];
		
		double timeWalk  = A / pow(adc, B);
//		double timeWalkU = A / pow(adcu, B);
		
		//		double tU = tInfos.time + d/ftc.veff[sector-1][panel-1][pmt][paddle-1]/cm + (1. - 2. * pmt)*ftc.toff_LR[sector-1][panel-1][paddle-1]/2.
		//		- ftc.toff_RFpad[sector-1][panel-1][paddle-1]
		//		- ftc.toff_P2P[sector-1][panel-1][paddle-1];
		
		double tU = tInfos.time + d/ftc.veff[sector-1][panel-1][pmt][paddle-1]/cm + (1. - 2. * pmt)*ftc.toff_LR[sector-1][panel-1][paddle-1]/2.
		- ftc.toff_RFpad[sector-1][panel-1][paddle-1];
		
		// cout << " FCAL Unsmeared Time before p2p subtraction: " << tU << endl;
		
		tU = tU - ftc.toff_P2P[sector-1][panel-1][paddle-1];
		
		// cout << " FCAL Unsmeared Time after p2p subtraction: " << tU << endl;
		
//		tdcu = (tU + timeWalkU) / tdcconv;
		tdc  = G4RandGauss::shoot(tU+ timeWalk, sqrt(2) * ftc.tres[sector - 1][panel - 1][paddle - 1]) / tdcconv;
		
	}
	
	
	// Status flags
	if(accountForHardwareStatus) {
		switch (ftc.status[sector - 1][panel - 1][pmt][paddle - 1]) {
		case 0:
			break;
		case 1:
			adc = 0;
			break;
		case 2:
			tdc = 0;
			break;
		case 3:
			adc = tdc = 0;
			break;
			
		case 5:
			break;
			
		default:
			cout << " > Unknown FCAL status: " << ftc.status[sector - 1][panel - 1][0][paddle - 1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left " << endl;
		}
	}
	
	//	cout << " > FCAL status: " << ftc.status[sector-1][panel-1][0][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left: " << adcl << endl;
	//	cout << " > FCAL status: " << ftc.status[sector-1][panel-1][1][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " right:  " << adcr << endl;
	
	dgtz["sector"]    = sector;
	dgtz["layer"]     = panel;
	dgtz["component"] = paddle;
	dgtz["ADC_order"] = pmt;
	dgtz["ADC_ADC"]   = (int) adc;
	dgtz["ADC_time"]  = (tdc*tdcconv);
	dgtz["ADC_ped"]   = 0;

	dgtz["TDC_order"] = pmt + 2;
	dgtz["TDC_TDC"]   = (int) tdc;


	// decide if write an hit or not
	writeHit = true;
	// define conditions to reject hit
	if (rejectHitConditions) {
		writeHit = false;
	}
	
	return dgtz;
}


// sector = identity[0].id;
// panel  = identity[1].id; // 1-1A, 2-1B, 3-2B
// paddle = identity[2].id;
// pmt    = identity[3].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide

vector<identifier> fcal_HitProcess::processID(vector<identifier> id, G4Step* aStep, detector Detector) {
  //cout << "fcal_HitProcess::processID" << endl;
	vector<identifier> yid = id;
	yid[0].id_sharing = 1; // sector
	yid[1].id_sharing = 1; // panel
	yid[2].id_sharing = 1; // paddle
	yid[3].id_sharing = 1; // side, left or right
	
	if (yid[3].id != 0) {
		cout << "*****WARNING***** in fcal_HitProcess :: processID, identifier PTT of the original hit should be 0 " << endl;
		cout << "yid[3].id = " << yid[3].id << endl;
	}
	
	// Now we want to have similar identifiers, but the only difference be id PMT to be 1, instead of 0
	identifier this_id = yid[0];
	yid.push_back(this_id);
	this_id = yid[1];
	yid.push_back(this_id);
	this_id = yid[2];
	yid.push_back(this_id);
	this_id = yid[3];
	this_id.id = 1;
	yid.push_back(this_id);
	
	return yid;
}

// - electronicNoise: returns a vector of hits generated / by electronics.

vector<MHit*> fcal_HitProcess::electronicNoise() {
  //cout << "fcal_HitProcess::electronicNoise" << endl;
	vector<MHit*> noiseHits;
	
	// first, identify the cells that would have electronic noise
	// then instantiate hit with energy E, time T, identifier IDF:
	//
	// MHit* thisNoiseHit = new MHit(E, T, IDF, pid);
	
	// push to noiseHits collection:
	// noiseHits.push_back(thisNoiseHit)
	
	return noiseHits;
}


map< string, vector <int> > fcal_HitProcess::multiDgt(MHit* aHit, int hitn) {
	map< string, vector <int> > MH;
	//cout << "fcal_HitProcess::multiDgt" << endl;
	return MH;
}

// - charge: returns charge/time digitized information / step

map< int, vector <double> > fcal_HitProcess::chargeTime(MHit* aHit, int hitn) {
	map< int, vector <double> > CT;
	//cout << "fcal_HitProcess::chargeTime" << endl;
	vector<double> hitNumbers;
	vector<double> stepIndex;
	vector<double> chargeAtElectronics;
	vector<double> timeAtElectronics;
	vector<double> identifiers;
	vector<double> hardware;
	hitNumbers.push_back(hitn);
	
	// getting identifiers
	vector<identifier> identity = aHit->GetId();
	
	int sector = identity[0].id;
	int panel = identity[1].id;
	int paddle = identity[2].id;
	int pmt = identity[3].id; // 0=> Left PMT, 1=> Right PMT. A better name would be pmtSide
	
	identifiers.push_back(sector); // sector
	identifiers.push_back(panel); // panel, 1a, 1b, 2a
	identifiers.push_back(paddle); // paddle number
	identifiers.push_back(pmt); // the pmt side: 0=> Left, 1=>Right
	
	// getting hardware
	Hardware thisHardware = ftc.TT.getHardware({sector, panel, paddle, pmt});
	hardware.push_back(thisHardware.getCrate());
	hardware.push_back(thisHardware.getSlot());
	hardware.push_back(thisHardware.getChannel());
	
	// Adding pedestal mean and sigma into the hardware as well
	// All of these variables start from 1, therefore -1 is subtracted, e.g. sector-1
	hardware.push_back(ftc.pedestal[sector - 1][panel - 1][paddle - 1][pmt]);
	hardware.push_back(ftc.pedestal_sigm[sector - 1][panel - 1 ][paddle - 1][pmt]);
	
	// attenuation length
	double attlen = ftc.attlen[sector - 1][panel - 1][pmt][paddle - 1];
	double attlen_otherside = ftc.attlen[sector - 1][panel - 1][1 - pmt].at(paddle - 1);
	
	trueInfos tInfos(aHit);
	
	// Get the paddle half-length
	double length = aHit->GetDetector().dimensions[0];
	
	// Vector of positions of the hit in each step
	vector<G4ThreeVector> Lpos = aHit->GetLPos();
	
	// Vector of Edep and time of the hit in each step
	vector<G4double> Edep = aHit->GetEdep();
	vector<G4double> time = aHit->GetTime();
	
	for (unsigned int s = 0; s < tInfos.nsteps; s++) {
		// Distances from left, right
		//	double dLeft  = length + tInfos.lx;
		//	double dRight = length - tInfos.lx;
		
		double d = length + (1 - 2 * pmt) * Lpos[s].x();
		//double d = length + (1 - 2 * pmt) * tInfos.lx;
		
		// attenuation factor
		//	double attLeft  = exp(-dLeft/cm/attlenL);
		//	double attRight = exp(-dRight/cm/attlenR);
		
		double att = exp(-d / cm / attlen);
		
		// Gain factors to simulate FCAL PMT gain matching algorithm.
		// Each L,R PMT pair has HV adjusted so geometeric mean sqrt(L*R)
		// is independent of counter length, which compensates for
		// the factor exp(-L/2/attlen) where L=full length of bar.
		//	double gainLeft  = sqrt(attLeft*attRight);
		//	double gainRight = gainLeft;
		
		double gain = sqrt(exp(-d / cm /attlen ) * exp(-(2 * length - d) / cm / attlen_otherside));
		
		// Attenuated light at PMT
		//	double eneL = tInfos.eTot*attLeft;
		//	double eneR = tInfos.eTot*attRight;
		
		double ene = Edep[s] * att;
		
		// giving geantinos some energies
		if (aHit->GetPID() == 0) {
			double gmomentum = aHit->GetMom().mag() / GeV;
			//		eneL = gmomentum*attLeft;
			//		eneR = gmomentum*attRight;
			
			ene = gmomentum*att;
		}
		
		double adc = 0;
		
		// Fluctuate the light measured by the PMT with
		// Poisson distribution for emitted photoelectrons
		// Treat L and R separately, in case nphe=0
		
		double nphe = G4Poisson(ene * ftc.pmtPEYld);
		ene = nphe / ftc.pmtPEYld;
		
		if (ene > 0) {
			adc = ene * ftc.countsForMIP[sector - 1][panel - 1][pmt][paddle - 1] / ftc.dEMIP[panel - 1] / gain;
			double A = ftc.twlk[sector - 1][panel - 1][3 * pmt + 0][paddle - 1];
			double B = ftc.twlk[sector - 1][panel - 1][3 * pmt + 1][paddle - 1];
			//double            C = ftc.twlk[sector-1][panel-1][2][paddle-1];
			double timeWalk = A / pow(adc, B);
			
			double stepTimeU = time[s] + d/ftc.veff[sector-1][panel-1][pmt][paddle-1]/cm + (1. - 2. * pmt)*ftc.toff_LR[sector-1][panel-1][paddle-1]/2.
			- ftc.toff_RFpad[sector-1][panel-1][paddle-1]
			- ftc.toff_P2P[sector-1][panel-1][paddle-1]
			+ timeWalk;
			
			double stepTime = G4RandGauss::shoot(stepTimeU, sqrt(2) * ftc.tres[sector - 1][panel - 1][paddle - 1]);
			
			stepIndex.push_back(s); // Since it is going to be only one hit, i.e. only one step
			chargeAtElectronics.push_back(adc);
			timeAtElectronics.push_back(stepTime);
		}
		
	}
	
	//	// Status flags
	//	switch (ftc.status[sector-1][panel-1][pmt][paddle-1])
	//	{
	//		case 0:
	//			break;
	//		case 1:
	//			adc = 0;
	//			break;
	//		case 2:
	//			tdc = 0;
	//			break;
	//		case 3:
	//			adc = tdc = 0;
	//			break;
	//
	//		case 5:
	//			break;
	//
	//		default:
	//			cout << " > Unknown FCAL status: " << ftc.status[sector-1][panel-1][0][paddle-1] << " for sector " << sector << ",  panel " << panel << ", paddle " << paddle << " left " << endl;
	//	}
	
	
	CT[0] = hitNumbers;
	CT[1] = stepIndex;
	CT[2] = chargeAtElectronics;
	CT[3] = timeAtElectronics;
	CT[4] = identifiers;
	CT[5] = hardware;
	
	return CT;
}

// - voltage: returns a voltage value for a given time. The inputs are:
// charge value (coming from chargeAtElectronics)
// time (coming from timeAtElectronics)

double fcal_HitProcess::voltage(double charge, double time, double forTime) {
  //cout << "fcal_HitProcess::voltage" << endl;
	//	return 0.0;
	return PulseShape(forTime, ftc.vpar, charge, time);
}

void fcal_HitProcess::initWithRunNumber(int runno)
{
  //cout << "fcal_HitProcess::initWithRunNumber" << endl;
	string digiVariation    = gemcOpt.optMap["DIGITIZATION_VARIATION"].args;
	string digiSnapshotTime = gemcOpt.optMap["DIGITIZATION_TIMESTAMP"].args;
	
	if (ftc.runNo != runno) {
		cout << " > Initializing " << HCname << " digitization for run number " << runno << endl;
		ftc = initializeFCALConstants(runno, digiVariation, digiSnapshotTime, accountForHardwareStatus);
		ftc.runNo = runno;
	}
}

// this static function will be loaded first thing by the executable
fcalConstants fcal_HitProcess::ftc = initializeFCALConstants(-1);




