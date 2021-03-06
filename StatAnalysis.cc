
#include "../interface/StatAnalysis.h"
#include "Sorters.h"
#include "PhotonReducedInfo.h"
#include <iostream>
#include <algorithm>
#include <stdio.h>

#define PADEBUG 0
#define FMDEBUG 0
#define FMDEBUG_1 0 

using namespace std;

void dumpPhoton(std::ostream & eventListText, int lab,
		LoopAll & l, int ipho, int ivtx, TLorentzVector & phop4, float * pho_energy_array);
void dumpJet(std::ostream & eventListText, int lab, LoopAll & l, int ijet);


// ----------------------------------------------------------------------------------------------------
StatAnalysis::StatAnalysis()  :
    name_("StatAnalysis")
{

    systRange  = 3.; // in units of sigma
    nSystSteps = 1;
    doSystematics = true;
    nVBFDijetJetCategories=2;
    scaleClusterShapes = true;
    dumpAscii = false;
    dumpMcAscii = false;
    unblind = false;
    doMcOptimization = false;

    nVBFCategories   = 0;
    nVHhadCategories = 0;
    nVHhadBtagCategories = 0;
    nVHlepCategories = 0;
    nVHmetCategories = 0;
    nTTHhadCategories = 0;
    nTTHlepCategories = 0;
    nTprimehadCategories = 0;//GIUSEPPE
    nTprimelepCategories = 0;//GIUSEPPE
    nLooseCategories = 0;//GIUSEPPE
    ntHqLeptonicCategories = 0;

    
    nVtxCategories = 0;
    R9CatBoundary = 0.94;

    fillOptTree = false;
    doFullMvaFinalTree = false;

    splitwzh=false;
    sigmaMrv=0.;
    sigmaMwv=0.;
}

// ----------------------------------------------------------------------------------------------------
StatAnalysis::~StatAnalysis()
{
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::Term(LoopAll& l)
{

    std::string outputfilename = (std::string) l.histFileName;
    cout<<"OUTFILENAME"<<outputfilename<<endl;
    // Make Fits to the data-sets and systematic sets
    std::string postfix=(dataIs2011?"":"_8TeV");
    l.rooContainer->FitToData("data_pol_model"+postfix,"data_mass");  // Fit to full range of dataset

    //    l.rooContainer->WriteSpecificCategoryDataCards(outputfilename,"data_mass","sig_mass","data_pol_model");
    //    l.rooContainer->WriteDataCard(outputfilename,"data_mass","sig_mass","data_pol_model");
    // mode 0 as above, 1 if want to bin in sub range from fit,

    // Write the data-card for the Combinations Code, needs the output filename, makes binned analysis DataCard
    // Assumes the signal datasets will be called signal_name+"_mXXX"
    //    l.rooContainer->GenerateBinnedPdf("bkg_mass_rebinned","data_pol_model","data_mass",1,50,1); // 1 means systematics from the fit effect only the backgroundi. last digit mode = 1 means this is an internal constraint fit
    //    l.rooContainer->WriteDataCard(outputfilename,"data_mass","sig_mass","bkg_mass_rebinned");

    eventListText.close();
    lep_sync.close();

    std::cout << " nevents " <<  nevents << " " << sumwei << std::endl;

    met_sync.close();

    //  kfacFile->Close();
    //  PhotonAnalysis::Term(l);
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::Init(LoopAll& l)
{
    if(PADEBUG)
        cout << "InitRealStatAnalysis START"<<endl;

    nevents=0., sumwei=0.;
    sumaccept=0., sumsmear=0., sumev=0.;

    met_sync.open ("met_sync.txt");

    std::string outputfilename = (std::string) l.histFileName;
    eventListText.open(Form("%s",l.outputTextFileName.c_str()));
    lep_sync.open ("lep_sync.txt");
    //eventListText.open(Form("%s_ascii_events.txt",outputfilename.c_str()));
    FillSignalLabelMap(l);
    //
    // These parameters are set in the configuration file
    std::cout
        << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << "StatAnalysis " << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << "leadEtCut "<< leadEtCut << "\n"
        << "subleadEtCut "<< subleadEtCut << "\n"
        << "doTriggerSelection "<< doTriggerSelection << "\n"
        << "nEtaCategories "<< nEtaCategories << "\n"
        << "nR9Categories "<< nR9Categories << "\n"
        << "nPtCategories "<< nPtCategories << "\n"
        << "doEscaleSyst "<< doEscaleSyst << "\n"
        << "doEresolSyst "<< doEresolSyst << "\n"
        << "doEcorrectionSyst "<< doEcorrectionSyst << "\n"
        << "efficiencyFile " << efficiencyFile << "\n"
        << "doPhotonIdEffSyst "<< doPhotonIdEffSyst << "\n"
        << "doR9Syst "<< doR9Syst << "\n"
        << "doVtxEffSyst "<< doVtxEffSyst << "\n"
        << "doTriggerEffSyst "<< doTriggerEffSyst << "\n"
        << "doKFactorSyst "<< doKFactorSyst << "\n"
        << "-------------------------------------------------------------------------------------- \n"
        << std::endl;

    // call the base class initializer
    PhotonAnalysis::Init(l);

    // Avoid reweighing from histo conainer
    for(size_t ind=0; ind<l.histoContainer.size(); ind++) {
        l.histoContainer[ind].setScale(1.);
    }

    diPhoCounter_ = l.countersred.size();
    l.countersred.resize(diPhoCounter_+1);

    // initialize the analysis variables
    nInclusiveCategories_ = nEtaCategories;
    if( nR9Categories != 0 ) nInclusiveCategories_ *= nR9Categories;
    if( nPtCategories != 0 ) nInclusiveCategories_ *= nPtCategories;

    // mva removed cp march 8
    //if( useMVA ) nInclusiveCategories_ = nDiphoEventClasses;

    // CP

    nPhotonCategories_ = nEtaCategories;
    if( nR9Categories != 0 ) nPhotonCategories_ *= nR9Categories;

    nVBFCategories   = ((int)includeVBF)*( (mvaVbfSelection && !multiclassVbfSelection) ? mvaVbfCatBoundaries.size()-1 : nVBFEtaCategories*nVBFDijetJetCategories );
    std::sort(mvaVbfCatBoundaries.begin(),mvaVbfCatBoundaries.end(), std::greater<float>() );
    if (multiclassVbfSelection) {
	std::vector<int> vsize;
	vsize.push_back((int)multiclassVbfCatBoundaries0.size());
	vsize.push_back((int)multiclassVbfCatBoundaries1.size());
	vsize.push_back((int)multiclassVbfCatBoundaries2.size());
	std::sort(vsize.begin(),vsize.end(), std::greater<int>());
	// sanity check: there sould be at least 2 vectors with size==2
       	if (vsize[0]<2 || vsize[1]<2 ){
	    std::cout << "Not enough category boundaries:" << std::endl;
	    std::cout << "multiclassVbfCatBoundaries0 size = " << multiclassVbfCatBoundaries0.size() << endl;
	    std::cout << "multiclassVbfCatBoundaries1 size = " << multiclassVbfCatBoundaries1.size() << endl;
	    std::cout << "multiclassVbfCatBoundaries2 size = " << multiclassVbfCatBoundaries2.size() << endl;
	    assert( 0 );
	}
	nVBFCategories   = vsize[0]-1;
	std::sort(multiclassVbfCatBoundaries0.begin(),multiclassVbfCatBoundaries0.end(), std::greater<float>() );
	std::sort(multiclassVbfCatBoundaries1.begin(),multiclassVbfCatBoundaries1.end(), std::greater<float>() );
	std::sort(multiclassVbfCatBoundaries2.begin(),multiclassVbfCatBoundaries2.end(), std::greater<float>() );
    }

    nVHhadCategories = ((int)includeVHhad)*nVHhadEtaCategories;
    nVHhadBtagCategories =((int)includeVHhadBtag);
    nTTHhadCategories =((int)includeTTHhad);
    nTTHlepCategories =((int)includeTTHlep);
    nTprimehadCategories =((int)includeTprimehad);//GIUSEPPE
    nTprimelepCategories =((int)includeTprimelep);//GIUSEPPE
    nLooseCategories =((int)includeLoose);//GIUSEPPE
    ntHqLeptonicCategories = ((int)includetHqLeptonic);

    if(includeVHlep){
        nVHlepCategories = nElectronCategories + nMuonCategories;
    }
    //--------------------------- ANIELLO --------------------------//
    if(includeVHlepB){
        nVHlepCategories = 2;
    }
    //-------------------------------------------------------------//
    nVHmetCategories = (int)includeVHmet;  //met at analysis step

    //    nCategories_=(nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories+nVHmetCategories);  //met at analysis step
//    nCategories_=(nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories);
    
//CONTROLLO NUMERO DI CATEGORIE:
nCategories_=(nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories+nVHmetCategories+nVHhadBtagCategories+nTTHhadCategories+nTTHlepCategories+ntHqLeptonicCategories+nTprimehadCategories+nTprimelepCategories+nLooseCategories);  //=>Tprime GIUSEPPE

    cout<<"Numero di categorie"<<nCategories_<<endl;
    cout<<"Numero di inclusive"<<nInclusiveCategories_<<endl;
    cout<<"Numero di VBF"<<nVBFCategories<<endl;
    cout<<"Numero di categorie VHhad"<<nVHhadCategories<<endl;
    cout<<"Numero di categorie VH lep"<<nVHlepCategories<<endl;
    cout<<"Numero di categorie VH met"<<nVHmetCategories<<endl;
    cout<<"Numero di categorie VH had Btag"<<nVHhadBtagCategories<<endl;
    cout<<"Numero di categorie TTh had"<<nTTHhadCategories<<endl;
    cout<<"Numero di categorie TTh lep"<<nTTHlepCategories<<endl;
    cout<<"Numero di categorie THq"<<ntHqLeptonicCategories<<endl;
    cout<<"Numero di categorie Tprime had"<<nTprimehadCategories<<endl;
    cout<<"Numero di categorie Tprime lep"<<nTprimelepCategories<<endl;
    cout<<"Numero di categorie Loose"<<nLooseCategories<<endl;

    effSmearPars.categoryType = effPhotonCategoryType;
    effSmearPars.n_categories = effPhotonNCat;
    effSmearPars.efficiency_file = efficiencyFile;

    diPhoEffSmearPars.n_categories = 8;
    diPhoEffSmearPars.efficiency_file = efficiencyFile;

    if( doEcorrectionSmear ) {
        // instance of this smearer done in PhotonAnalysis
        photonSmearers_.push_back(eCorrSmearer);
    }
    if( doEscaleSmear ) {
        setupEscaleSmearer();
    }
    if( doEresolSmear ) {
	setupEresolSmearer();
    }
    if( doPhotonIdEffSmear ) {
        // photon ID efficiency
        std::cerr << __LINE__ << std::endl;
        idEffSmearer = new EfficiencySmearer( effSmearPars );
        idEffSmearer->name("idEff");
        idEffSmearer->setEffName("ratioTP");
        idEffSmearer->init();
        idEffSmearer->doPhoId(true);
        photonSmearers_.push_back(idEffSmearer);
    }
    if( doR9Smear ) {
        // R9 re-weighting
        r9Smearer = new EfficiencySmearer( effSmearPars );
        r9Smearer->name("r9Eff");
        r9Smearer->setEffName("ratioR9");
        r9Smearer->init();
        r9Smearer->doR9(true);
        photonSmearers_.push_back(r9Smearer);
    }
    if( doVtxEffSmear ) {
        // Vertex ID
        std::cerr << __LINE__ << std::endl;
        vtxEffSmearer = new DiPhoEfficiencySmearer( diPhoEffSmearPars );   // triplicate TF1's here
        vtxEffSmearer->name("vtxEff");
        vtxEffSmearer->setEffName("ratioVertex");
        vtxEffSmearer->doVtxEff(true);
        vtxEffSmearer->init();
        diPhotonSmearers_.push_back(vtxEffSmearer);
    }
    if( doTriggerEffSmear ) {
        // trigger efficiency
        std::cerr << __LINE__ << std::endl;
        triggerEffSmearer = new DiPhoEfficiencySmearer( diPhoEffSmearPars );
        triggerEffSmearer->name("triggerEff");
        triggerEffSmearer->setEffName("effL1HLT");
        triggerEffSmearer->doVtxEff(false);
        triggerEffSmearer->init();
        diPhotonSmearers_.push_back(triggerEffSmearer);
    }
    if(doKFactorSmear) {
        // kFactor efficiency
        std::cerr << __LINE__ << std::endl;
        kFactorSmearer = new KFactorSmearer( kfacHist );
        kFactorSmearer->name("kFactor");
        kFactorSmearer->init();
        genLevelSmearers_.push_back(kFactorSmearer);
    }
    if(doInterferenceSmear) {
        // interference efficiency
        std::cerr << __LINE__ << std::endl;
        interferenceSmearer = new InterferenceSmearer(2.5e-2,0.);
        genLevelSmearers_.push_back(interferenceSmearer);
    }

    // Define the number of categories for the statistical analysis and
    // the systematic sets to be formed

    // FIXME move these params to config file
    l.rooContainer->SetNCategories(nCategories_);
    l.rooContainer->nsigmas = nSystSteps;
    l.rooContainer->sigmaRange = systRange;

    if( doEcorrectionSmear && doEcorrectionSyst ) {
        // instance of this smearer done in PhotonAnalysis
        systPhotonSmearers_.push_back(eCorrSmearer);
        std::vector<std::string> sys(1,eCorrSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doEscaleSmear && doEscaleSyst ) {
	setupEscaleSyst(l);
        //// systPhotonSmearers_.push_back( eScaleSmearer );
        //// std::vector<std::string> sys(1,eScaleSmearer->name());
        //// std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        //// l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doEresolSmear && doEresolSyst ) {
	setupEresolSyst(l);
        //// systPhotonSmearers_.push_back( eResolSmearer );
        //// std::vector<std::string> sys(1,eResolSmearer->name());
        //// std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        //// l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doPhotonIdEffSmear && doPhotonIdEffSyst ) {
        systPhotonSmearers_.push_back( idEffSmearer );
        std::vector<std::string> sys(1,idEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doR9Smear && doR9Syst ) {
        systPhotonSmearers_.push_back( r9Smearer );
        std::vector<std::string> sys(1,r9Smearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doVtxEffSmear && doVtxEffSyst ) {
        systDiPhotonSmearers_.push_back( vtxEffSmearer );
        std::vector<std::string> sys(1,vtxEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if( doTriggerEffSmear && doTriggerEffSyst ) {
        systDiPhotonSmearers_.push_back( triggerEffSmearer );
        std::vector<std::string> sys(1,triggerEffSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }
    if(doKFactorSmear && doKFactorSyst) {
        systGenLevelSmearers_.push_back(kFactorSmearer);
        std::vector<std::string> sys(1,kFactorSmearer->name());
        std::vector<int> sys_t(1,-1);   // -1 for signal, 1 for background 0 for both
        l.rooContainer->MakeSystematicStudy(sys,sys_t);
    }

    // ----------------------------------------------------
    // ----------------------------------------------------
    // Global systematics - Lumi
    l.rooContainer->AddGlobalSystematic("lumi",1.045,1.00);
    // ----------------------------------------------------

    // Create observables for shape-analysis with ranges
    // l.rooContainer->AddObservable("mass" ,100.,150.);
    l.rooContainer->AddObservable("CMS_hgg_mass" ,massMin,massMax);

    l.rooContainer->AddConstant("IntLumi",l.intlumi_);

    // SM Model
    l.rooContainer->AddConstant("XSBR_tth_155",0.00004370);
    l.rooContainer->AddConstant("XSBR_ggh_150",0.01428);
    l.rooContainer->AddConstant("XSBR_vbf_150",0.001308);
    l.rooContainer->AddConstant("XSBR_wzh_150",0.000641);
    l.rooContainer->AddConstant("XSBR_tth_150",0.000066);
    l.rooContainer->AddConstant("XSBR_ggh_145",0.018820);
    l.rooContainer->AddConstant("XSBR_vbf_145",0.001676);
    l.rooContainer->AddConstant("XSBR_wzh_145",0.000891);
    l.rooContainer->AddConstant("XSBR_tth_145",0.000090);
    l.rooContainer->AddConstant("XSBR_ggh_140",0.0234109);
    l.rooContainer->AddConstant("XSBR_vbf_140",0.00203036);
    l.rooContainer->AddConstant("XSBR_wzh_140",0.001163597);
    l.rooContainer->AddConstant("XSBR_tth_140",0.000117189);
    l.rooContainer->AddConstant("XSBR_ggh_135",0.0278604);
    l.rooContainer->AddConstant("XSBR_vbf_135",0.002343);
    l.rooContainer->AddConstant("XSBR_wzh_135",0.001457559);
    l.rooContainer->AddConstant("XSBR_tth_135",0.000145053);
    l.rooContainer->AddConstant("XSBR_ggh_130",0.0319112);
    l.rooContainer->AddConstant("XSBR_vbf_130",0.00260804);
    l.rooContainer->AddConstant("XSBR_wzh_130",0.001759636);
    l.rooContainer->AddConstant("XSBR_tth_130",0.000173070);
    l.rooContainer->AddConstant("XSBR_ggh_125",0.0350599);
    l.rooContainer->AddConstant("XSBR_vbf_125",0.00277319);
    l.rooContainer->AddConstant("XSBR_wzh_125",0.002035123);
    l.rooContainer->AddConstant("XSBR_tth_125",0.000197718);

    l.rooContainer->AddConstant("XSBR_TprimeM400_120",0.00524638);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM450_120",0.00253536);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM500_120",0.00180348);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM550_120",0.0006954);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM700_120",0.000129732);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM800_120",0.000047424);// GIUSEPPE
    l.rooContainer->AddConstant("XSBR_TprimeM900_120",0.0000184452);// GIUSEPPE

    l.rooContainer->AddConstant("XSBR_ggh_120",0.0374175);
    l.rooContainer->AddConstant("XSBR_vbf_120",0.00285525);
    l.rooContainer->AddConstant("XSBR_wzh_120",0.002285775);
    l.rooContainer->AddConstant("XSBR_tth_120",0.00021951);
    l.rooContainer->AddConstant("XSBR_ggh_123",0.0360696);
    l.rooContainer->AddConstant("XSBR_vbf_123",0.00281352);
    l.rooContainer->AddConstant("XSBR_wzh_123",0.00213681);
    l.rooContainer->AddConstant("XSBR_tth_123",0.00020663);
    l.rooContainer->AddConstant("XSBR_ggh_121",0.0369736);
    l.rooContainer->AddConstant("XSBR_vbf_121",0.00284082);
    l.rooContainer->AddConstant("XSBR_wzh_121",0.00223491);
    l.rooContainer->AddConstant("XSBR_tth_121",0.00021510);
    l.rooContainer->AddConstant("XSBR_ggh_115",0.0386169);
    l.rooContainer->AddConstant("XSBR_vbf_115",0.00283716);
    l.rooContainer->AddConstant("XSBR_wzh_115",0.002482089);
    l.rooContainer->AddConstant("XSBR_tth_115",0.000235578);
    l.rooContainer->AddConstant("XSBR_ggh_110",0.0390848);
    l.rooContainer->AddConstant("XSBR_vbf_110",0.00275406);
    l.rooContainer->AddConstant("XSBR_wzh_110",0.002654575);
    l.rooContainer->AddConstant("XSBR_tth_110",0.000247629);
    l.rooContainer->AddConstant("XSBR_ggh_105",0.0387684);
    l.rooContainer->AddConstant("XSBR_vbf_105",0.00262016);
    l.rooContainer->AddConstant("XSBR_wzh_105",0.002781962);
    l.rooContainer->AddConstant("XSBR_tth_105",0.000255074);

    // FF model
    l.rooContainer->AddConstant("ff_XSBR_vbf_150",0.00259659);
    l.rooContainer->AddConstant("ff_XSBR_wzh_150",0.00127278);
    l.rooContainer->AddConstant("ff_XSBR_vbf_145",0.00387544);
    l.rooContainer->AddConstant("ff_XSBR_wzh_145",0.00205969);
    l.rooContainer->AddConstant("ff_XSBR_vbf_140",0.00565976);
    l.rooContainer->AddConstant("ff_XSBR_wzh_140",0.003243602);
    l.rooContainer->AddConstant("ff_XSBR_vbf_135",0.00825);
    l.rooContainer->AddConstant("ff_XSBR_wzh_135",0.00513225);
    l.rooContainer->AddConstant("ff_XSBR_vbf_130",0.0122324);
    l.rooContainer->AddConstant("ff_XSBR_wzh_130",0.00825316);
    l.rooContainer->AddConstant("ff_XSBR_vbf_125",0.0186494);
    l.rooContainer->AddConstant("ff_XSBR_wzh_125",0.01368598);
    l.rooContainer->AddConstant("ff_XSBR_vbf_123",0.022212);
    l.rooContainer->AddConstant("ff_XSBR_wzh_123",0.0168696);
    l.rooContainer->AddConstant("ff_XSBR_vbf_121",0.0266484);
    l.rooContainer->AddConstant("ff_XSBR_wzh_121",0.0209646);
    l.rooContainer->AddConstant("ff_XSBR_vbf_120",0.0293139);
    l.rooContainer->AddConstant("ff_XSBR_wzh_120",0.02346729);
    l.rooContainer->AddConstant("ff_XSBR_vbf_115",0.0482184);
    l.rooContainer->AddConstant("ff_XSBR_wzh_115",0.04218386);
    l.rooContainer->AddConstant("ff_XSBR_vbf_110",0.083181);
    l.rooContainer->AddConstant("ff_XSBR_wzh_110",0.08017625);
    l.rooContainer->AddConstant("ff_XSBR_vbf_105",0.151616);
    l.rooContainer->AddConstant("ff_XSBR_wzh_105",0.1609787);

    // -----------------------------------------------------
    // Configurable background model
    // if no configuration was given, set some defaults
    std::string postfix=(dataIs2011?"":"_8TeV");

    if( bkgPolOrderByCat.empty() ) {//MAYBE IS USELESS???GIUSEPPE
	    for(int i=0; i<nCategories_; i++){
	        if(i<nInclusiveCategories_) {
	    	bkgPolOrderByCat.push_back(5);
	        } else if(i<nInclusiveCategories_+nVBFCategories){
	    	bkgPolOrderByCat.push_back(3);
	        } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories){
	    	bkgPolOrderByCat.push_back(2);
	        } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHlepCategories){
	    	bkgPolOrderByCat.push_back(1);
	        } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHhadBtagCategories+nVHlepCategories+nTTHhadCategories+nTTHlepCategories){
	    	bkgPolOrderByCat.push_back(3);
	        } else if(i<nInclusiveCategories_+nVBFCategories+nVHhadCategories+nVHhadBtagCategories+nVHlepCategories+nTTHhadCategories+nTTHlepCategories+ntHqLeptonicCategories){
	    	bkgPolOrderByCat.push_back(3);
		}
	    }
    }
    // build the model
    buildBkgModel(l, postfix);

    // -----------------------------------------------------
    // Make some data sets from the observables to fill in the event loop
    // Binning is for histograms (will also produce unbinned data sets)
    l.rooContainer->CreateDataSet("CMS_hgg_mass","data_mass"    ,nDataBins); // (100,110,150) -> for a window, else full obs range is taken
    l.rooContainer->CreateDataSet("CMS_hgg_mass","bkg_mass"     ,nDataBins);

    // Create Signal DataSets:
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
	int sig = sigPointsToBook[isig];
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM400_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM450_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM500_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM550_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM700_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM800_mass_m%d",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM900_mass_m%d",sig),nDataBins);//GIUSEPPE

        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_rv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_rv",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_rv",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_rv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_rv",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_rv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM400_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM450_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM500_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM550_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM700_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM800_mass_m%d_rv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM900_mass_m%d_rv",sig),nDataBins);//GIUSEPPE

        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_ggh_mass_m%d_wv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_vbf_mass_m%d_wv",sig),nDataBins);
        if(!splitwzh) l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wzh_mass_m%d_wv",sig),nDataBins);
        else{
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_wh_mass_m%d_wv",sig),nDataBins);
            l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_zh_mass_m%d_wv",sig),nDataBins);
        }
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_tth_mass_m%d_wv",sig),nDataBins);
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM400_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM450_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM500_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM550_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM700_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM800_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
        l.rooContainer->CreateDataSet("CMS_hgg_mass",Form("sig_TprimeM900_mass_m%d_wv",sig),nDataBins);//GIUSEPPE
    }

    // Make more datasets representing Systematic Shifts of various quantities
    for(size_t isig=0; isig<sigPointsToBook.size(); ++isig) {
	int sig = sigPointsToBook[isig];
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_ggh_mass_m%d",sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_vbf_mass_m%d",sig),-1);
        if(!splitwzh) l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wzh_mass_m%d",sig),-1);
        else{
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_wh_mass_m%d",sig),-1);
            l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_zh_mass_m%d",sig),-1);
        }
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_tth_mass_m%d",sig),-1);
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM400_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM450_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM500_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM550_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM700_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM800_mass_m%d",sig),-1);//GIUSEPPE
        l.rooContainer->MakeSystematics("CMS_hgg_mass",Form("sig_TprimeM900_mass_m%d",sig),-1);//GIUSEPPE
    }

    // Make sure the Map is filled
    FillSignalLabelMap(l);

    if(PADEBUG)
        cout << "InitRealStatAnalysis END"<<endl;

    // FIXME book of additional variables
}


// ----------------------------------------------------------------------------------------------------
void StatAnalysis::buildBkgModel(LoopAll& l, const std::string & postfix)
{

    // sanity check
    if( bkgPolOrderByCat.size() != nCategories_ ) {
	std::cout << "Number of categories not consistent with specified background model " << nCategories_ << " " << bkgPolOrderByCat.size() << std::endl;
	assert( 0 );
    }

    l.rooContainer->AddRealVar("CMS_hgg_pol6_0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_4"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol6_5"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_0"+postfix,"@0*@0","CMS_hgg_pol6_0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_1"+postfix,"@0*@0","CMS_hgg_pol6_1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_2"+postfix,"@0*@0","CMS_hgg_pol6_2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_3"+postfix,"@0*@0","CMS_hgg_pol6_3"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_4"+postfix,"@0*@0","CMS_hgg_pol6_4"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol6_5"+postfix,"@0*@0","CMS_hgg_pol6_4"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_pol5_0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_pol5_4"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_0"+postfix,"@0*@0","CMS_hgg_pol5_0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_1"+postfix,"@0*@0","CMS_hgg_pol5_1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_2"+postfix,"@0*@0","CMS_hgg_pol5_2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_3"+postfix,"@0*@0","CMS_hgg_pol5_3"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modpol5_4"+postfix,"@0*@0","CMS_hgg_pol5_4"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_quartic0"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic1"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic2"+postfix,-0.1,-1.0,1.0);
    l.rooContainer->AddRealVar("CMS_hgg_quartic3"+postfix,-0.01,-1.0,1.0);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic0"+postfix,"@0*@0","CMS_hgg_quartic0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic1"+postfix,"@0*@0","CMS_hgg_quartic1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic2"+postfix,"@0*@0","CMS_hgg_quartic2"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquartic3"+postfix,"@0*@0","CMS_hgg_quartic3"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_quad0"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_quad1"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquad0"+postfix,"@0*@0","CMS_hgg_quad0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modquad1"+postfix,"@0*@0","CMS_hgg_quad1"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_cubic0"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_cubic1"+postfix,-0.1,-1.5,1.5);
    l.rooContainer->AddRealVar("CMS_hgg_cubic2"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic0"+postfix,"@0*@0","CMS_hgg_cubic0"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic1"+postfix,"@0*@0","CMS_hgg_cubic1"+postfix);
    l.rooContainer->AddFormulaVar("CMS_hgg_modcubic2"+postfix,"@0*@0","CMS_hgg_cubic2"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_lin0"+postfix,-0.01,-1.5,1.5);
    l.rooContainer->AddFormulaVar("CMS_hgg_modlin0"+postfix,"@0*@0","CMS_hgg_lin0"+postfix);

    l.rooContainer->AddRealVar("CMS_hgg_plaw0"+postfix,0.01,-10,10);

    l.rooContainer->AddRealVar("CMS_hgg_exp0"+postfix,-1,-10,0);//exp model

    // prefix for models parameters
    std::map<int,std::string> parnames;
    parnames[1] = "modlin";
    parnames[2] = "modquad";
    parnames[3] = "modcubic";
    parnames[4] = "modquartic";
    parnames[5] = "modpol5_";
    parnames[6] = "modpol6_";
    parnames[-1] = "plaw";
    parnames[-10] = "exp";//exp model

    // map order to categories flags + parameters names
    std::map<int, std::pair<std::vector<int>, std::vector<std::string> > > catmodels;
    // fill the map
    for(int icat=0; icat<nCategories_; ++icat) {
        // get the poly order for this category
        int catmodel = bkgPolOrderByCat[icat];
        std::vector<int> & catflags = catmodels[catmodel].first;
        std::vector<std::string> & catpars = catmodels[catmodel].second;
        // if this is the first time we find this order, build the parameters
        if( catflags.empty() ) {
            assert( catpars.empty() );
            // by default no category has the new model
            catflags.resize(nCategories_, 0);
            std::string & parname = parnames[catmodel];
            if( catmodel > 0 ) {
                for(int iorder = 0; iorder<catmodel; ++iorder) {
                    catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), iorder, +postfix.c_str() ) );
                }
            } else {
				cout<<"catmodel:"<<catmodel<<endl;
                if( catmodel != -1 && catmodel != -10 ) {
                    std::cout << "The only supported negative bkg poly orders are -1 and -10, ie 1-parmeter power law -10 exponential" << std::endl;
                    assert( 0 );
                }
		if(catmodel == -1 ){
		    catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), 0, +postfix.c_str() ) );
		}else if(catmodel == -10 ){
		    catpars.push_back( Form( "CMS_hgg_%s%d%s", parname.c_str(), 0, +postfix.c_str() ) );
		}

            }
        } else if ( catmodel != -1 && catmodel != -10) {
            assert( catflags.size() == nCategories_ && catpars.size() == catmodel );
        }
        // chose category order
        catflags[icat] = 1;
    }


    // now loop over the models and allocate the pdfs
    /// for(size_t imodel=0; imodel<catmodels.size(); ++imodel ) {
    for(std::map<int, std::pair<std::vector<int>, std::vector<std::string> > >::iterator modit = catmodels.begin();
    modit!=catmodels.end(); ++modit ) {
        std::vector<int> & catflags = modit->second.first;
        std::vector<std::string> & catpars = modit->second.second;

        if( modit->first > 0 ) {
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                "0","CMS_hgg_mass",catpars,70+catpars.size());
            // >= 71 means RooBernstein of order >= 1
        }  else if (modit->first == -1){
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                "0","CMS_hgg_mass",catpars,6);
            // 6 is power law
        }else{
            l.rooContainer->AddSpecificCategoryPdf(&catflags[0],"data_pol_model"+postfix,
                "0","CMS_hgg_mass",catpars,1);
	    // 1 is exp
	}
    }
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::Analysis(LoopAll& l, Int_t jentry)//MARKER
{
    if(jentry==32534){    cout<<"Analysis di StatAnalysis.cc"<<endl;}
    if(PADEBUG)
        cout << "Analysis START; cur_type is: " << l.itype[l.current] <<endl;

    int cur_type = l.itype[l.current];
    float weight = l.sampleContainer[l.current_sample_index].weight;
    float sampleweight = l.sampleContainer[l.current_sample_index].weight;
    if(jentry==32534){    cout<<"dopo l.current"<<endl;}
    // Set reRunCiC Only if this is an MC event since scaling of R9 and Energy isn't done at reduction
    if (cur_type==0) {
        l.runCiC=reRunCiCForData;
    } else {
        l.runCiC = true;
    }
    if (l.runZeeValidation) l.runCiC=true;

    // make sure that rho is properly set
    if( dataIs2011 ) {
	l.version = 12;
    }
    if( l.version >= 13 && forcedRho < 0. ) {
	l.rho = l.rho_algo1;
    }

    l.FillCounter( "Processed", 1. );
    if( weight <= 0. ) {
	std::cout << "Zero or negative weight " << cur_type << " " << weight << std::endl;
	assert( 0 );
    }
    l.FillCounter( "XSWeighted", weight );
    nevents+=1.;
    if(jentry==32534){cout<<"prima di PU"<<endl;}
    //PU reweighting
    double pileupWeight=getPuWeight( l.pu_n, cur_type, &(l.sampleContainer[l.current_sample_index]), jentry == 1);
    sumwei +=pileupWeight;
    weight *= pileupWeight;
    sumev  += weight;

    assert( weight >= 0. );
    l.FillCounter( "PUWeighted", weight );

    if( jentry % 1000 ==  0 ) {
        std::cout << " nevents " <<  nevents << " sumpuweights " << sumwei << " ratio " << sumwei / nevents
                  << " equiv events " << sumev << " accepted " << sumaccept << " smeared " << sumsmear << " "
                  <<  sumaccept / sumev << " " << sumsmear / sumaccept
                  << std::endl;
    }
    // ------------------------------------------------------------
    //PT-H K-factors
    double gPT = 0;
    TLorentzVector gP4(0,0,0,0);

    if (!l.runZeeValidation && cur_type<0){
	gP4 = l.GetHiggs();
	gPT = gP4.Pt();
	//assert( gP4.M() > 0. );
    }
    if(jentry==32534){cout<<"prima di cluster shape variables"<<endl;}
    //Calculate cluster shape variables prior to shape rescaling
    for (int ipho=0;ipho<l.pho_n;ipho++){
	//// l.pho_s4ratio[ipho]  = l.pho_e2x2[ipho]/l.pho_e5x5[ipho];
	l.pho_s4ratio[ipho] = l.pho_e2x2[ipho]/l.bc_s25[l.sc_bcseedind[l.pho_scind[ipho]]];
	float rr2=l.pho_eseffsixix[ipho]*l.pho_eseffsixix[ipho]+l.pho_eseffsiyiy[ipho]*l.pho_eseffsiyiy[ipho];
	l.pho_ESEffSigmaRR[ipho] = 0.0;
	if(rr2>0. && rr2<999999.) {
	    l.pho_ESEffSigmaRR[ipho] = sqrt(rr2);
	}
    }
    if(jentry==32534){cout<<"prima di MC corrections"<<endl;}
    // Data driven MC corrections to cluster shape variables and energy resolution estimate
    if (cur_type !=0 && scaleClusterShapes ){
        rescaleClusterVariables(l);
    }
    if( useDefaultVertex ) {
	for(int id=0; id<l.dipho_n; ++id ) {
	    l.dipho_vtxind[id] = 0;
	}
    } else if( reRunVtx ) {
	reVertex(l);
    }
    if(jentry==32534){cout<<"prima di re-apply jec"<<endl;}
    // Re-apply JEC and / or recompute JetID
    if(includeVBF || includeVHhad || includeVHhadBtag || includeTTHhad || includeTTHlep || includetHqLeptonic ||includeTprimehad || includeTprimelep || includeLoose) { postProcessJets(l); }
    if(jentry==32534){cout<<"prima di Analyse the event"<<endl;}
    // Analyse the event assuming nominal values of corrections and smearings
    float mass, evweight, diphotonMVA;
    int diphoton_id, category;
    bool isCorrectVertex;
    bool storeEvent = false;
    if(jentry==32534||jentry==32533){cout<<"category"<<category<<endl;AnalyseEvent(l,jentry, weight, gP4, mass,  evweight, category, diphoton_id, isCorrectVertex,diphotonMVA); cout<<"dopo la valutazione AnalyseEvent"<<endl;}
    if( AnalyseEvent(l,jentry, weight, gP4, mass,  evweight, category, diphoton_id, isCorrectVertex,diphotonMVA) ) {
	// feed the event to the RooContainer
	if(jentry==32534){cout<<"dentro if ANAlyseEvent"<<endl;}
    FillRooContainer(l, cur_type, mass, diphotonMVA, category, evweight, isCorrectVertex, diphoton_id);
    	storeEvent = true;
    }
    if(jentry==32534){cout<<"prima di syste uncert"<<endl;}
    // Systematics uncertaities for the binned model
    // We re-analyse the event several times for different values of corrections and smearings
    if( cur_type < 0 && doMCSmearing && doSystematics ) {

        // fill steps for syst uncertainty study
        float systStep = systRange / (float)nSystSteps;

	float syst_mass, syst_weight, syst_diphotonMVA;
	int syst_category;
	std::vector<double> mass_errors;
	std::vector<double> mva_errors;
	std::vector<double> weights;
	std::vector<int>    categories;
    if(jentry==32534){cout<<"prima di diphoton"<<endl;}
        if (diphoton_id > -1 ) {

	    // gen-level systematics, i.e. ggH k-factor for the moment
            for(std::vector<BaseGenLevelSmearer*>::iterator si=systGenLevelSmearers_.begin(); si!=systGenLevelSmearers_.end(); si++){
		mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();

                for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                    if( syst_shift == 0. ) { continue; } // skip the central value
		    syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

		    // re-analyse the event without redoing the event selection as we use nominal values for the single photon
		    // corrections and smearings
		    AnalyseEvent(l, jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id, isCorrectVertex,syst_diphotonMVA,
				 true, syst_shift, true, *si, 0, 0 );

		    AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
				    mass_errors, mva_errors, categories, weights);
		}

		FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights,diphoton_id);
	    }
    if(jentry==32534){cout<<"prima di vertex efficiency"<<endl;}
	    // di-photon systematics: vertex efficiency and trigger
	    for(std::vector<BaseDiPhotonSmearer *>::iterator si=systDiPhotonSmearers_.begin(); si!= systDiPhotonSmearers_.end(); ++si ) {
		mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();

                for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
                    if( syst_shift == 0. ) { continue; } // skip the central value
		    syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

		    // re-analyse the event without redoing the event selection as we use nominal values for the single photon
		    // corrections and smearings
		    AnalyseEvent(l,jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id, isCorrectVertex,syst_diphotonMVA,
				 true, syst_shift, true,  0, 0, *si );

		    AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
                                    mass_errors, mva_errors, categories, weights);
		}

		FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights, diphoton_id);
	    }
	}

	int diphoton_id_syst;
    if(jentry==32534){cout<<"prima di single photon level syst"<<endl;}
	// single photon level systematics: several
	for(std::vector<BaseSmearer *>::iterator  si=systPhotonSmearers_.begin(); si!= systPhotonSmearers_.end(); ++si ) {
	    mass_errors.clear(), weights.clear(), categories.clear(), mva_errors.clear();

	    for(float syst_shift=-systRange; syst_shift<=systRange; syst_shift+=systStep ) {
		if( syst_shift == 0. ) { continue; } // skip the central value
		syst_mass     =  0., syst_category = -1, syst_weight   =  0.;

		// re-analyse the event redoing the event selection this time
		AnalyseEvent(l,jentry, weight, gP4, syst_mass,  syst_weight, syst_category, diphoton_id_syst, isCorrectVertex,syst_diphotonMVA,
			     true, syst_shift, false,  0, *si, 0 );

		AccumulateSyst( cur_type, syst_mass, syst_diphotonMVA, syst_category, syst_weight,
				mass_errors, mva_errors, categories, weights);
	    }

	    FillRooContainerSyst(l, (*si)->name(), cur_type, mass_errors, mva_errors, categories, weights, diphoton_id);
	}
    }

    if(PADEBUG)
        cout<<"myFillHistRed END"<<endl;
    if(jentry==32534){cout<<"prima di return"<<endl;}
    return storeEvent;
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::AnalyseEvent(LoopAll& l, Int_t jentry, float weight, TLorentzVector & gP4,
				float & mass, float & evweight, int & category, int & diphoton_id, bool & isCorrectVertex,
				float &kinematic_bdtout,
				bool isSyst,
				float syst_shift, bool skipSelection,
				BaseGenLevelSmearer *genSys, BaseSmearer *phoSys, BaseDiPhotonSmearer * diPhoSys)
{//MARK 2
    assert( isSyst || ! skipSelection );

    l.createCS_=createCS;

    int cur_type = l.itype[l.current];
    float sampleweight = l.sampleContainer[l.current_sample_index].weight;
    /// diphoton_id = -1;

    std::pair<int,int> diphoton_index;
    vbfIjet1=-1, vbfIjet2=-1;

    // do gen-level dependent first (e.g. k-factor); only for signal
    genLevWeight=1.;
    if(cur_type!=0 ) {
	applyGenLevelSmearings(genLevWeight,gP4,l.pu_n,cur_type,genSys,syst_shift);
    }
    cout<<"dentro AnalyseEvents di StatAnalysis.cc"<<endl;
    // event selection
    int muVtx=-1;
    int mu_ind=-1;
    int elVtx=-1;
    int el_ind=-1;

    int leadpho_ind=-1;
    int subleadpho_ind=-1;

    //-------------------------------- ANIELLO --------------------------------//
    bool VHmuevent_prov=false;
    bool VHelevent_prov=false;
    //-------------------------------------------------------------------------//
    cout<<"prima di !skipSelection"<<endl;
    if( ! skipSelection ) {
	cout<<"dentro !skipSelection"<<endl;
	    // first apply corrections and smearing on the single photons
	    smeared_pho_energy.clear(); smeared_pho_energy.resize(l.pho_n,0.);
	    smeared_pho_r9.clear();     smeared_pho_r9.resize(l.pho_n,0.);
	    smeared_pho_weight.clear(); smeared_pho_weight.resize(l.pho_n,1.);
	    cout<<"prima di applySinglePhoton"<<endl;

	    applySinglePhotonSmearings(smeared_pho_energy, smeared_pho_r9, smeared_pho_weight, cur_type, l, energyCorrected, energyCorrectedError,
				       phoSys, syst_shift);//CHE FA? Se non serve lo butto!
	    
cout<<"dopo applySinglePhoton"<<endl;
	    // Fill CiC efficiency plots for ggH, mH=124
	    //fillSignalEfficiencyPlots(weight, l);

	    // inclusive category di-photon selection
	    // FIXME pass smeared R9
        std::vector<bool> veto_indices;
	cout<<"prima di DiphotonCiCSlection"<<endl;//gia non arriva proprio qui
        veto_indices.clear();
	    diphoton_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0], false, -1, veto_indices, cicCutLevels );
	    cout<<"dopo diphoton_id dentro !skipSelection"<<endl;
	    //// diphoton_id = l.DiphotonCiCSelection(l.phoNOCUTS, l.phoNOCUTS, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0] );
	    cout<<"prima di ! isSyst"<<endl;
	    // N-1 plots
	    if( ! isSyst ) {
		cout<<"dentro N-1 plots"<<endl;
	        int diphoton_nm1_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoNOCUTS, leadEtCut, subleadEtCut, 4,applyPtoverM, &smeared_pho_energy[0] );
	        if(diphoton_nm1_id>-1) {
	    	float eventweight = weight * smeared_pho_weight[diphoton_index.first] * smeared_pho_weight[diphoton_index.second] * genLevWeight;
	    	float myweight=1.;
	    	if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;
	    	ClassicCatsNm1Plots(l, diphoton_nm1_id, &smeared_pho_energy[0], eventweight, myweight);
	        }
	    }
	    cout<<"prima di Excluse Modes"<<endl;
	    // Exclusive Modes
	    int diphotonVBF_id = -1;
	    int diphotonVHhad_id = -1;
	    int diphotonVHhadBtag_id = -1;
	    int diphotonTTHhad_id = -1;
	    int diphotonTTHlep_id = -1;
	    int diphotonTprimehad_id = -1;//GIUSEPPE
	    int diphotonTprimelep_id = -1;//GIUSEPPE
	    int diphotonLoose_id = -1;//GIUSEPPE
	    int diphotonVHlep_id = -1;
	    int diphotonVHmet_id = -1; //met at analysis step
	    int diphotontHqLeptonic_id = -1;
	    VHmuevent = false;
	    VHelevent = false;
	    VHlep1event = false; //ANIELLO
	    VHlep2event = false; //ANIELLO
	    VHmuevent_cat = 0;
	    VHelevent_cat = 0;
	    VBFevent = false;
	    VHhadevent = false;
	    VHhadBtagevent = false;
	    TTHhadevent = false;
	    TTHlepevent = false;
	    Tprimehadevent = false;//GIUSEPPE
	    Tprimelepevent = false;//GIUSEPPE
	    Looseevent = false;//GIUSEPPE
	    VHmetevent = false; //met at analysis step
	    tHqLeptonicevent = false;

	    // lepton tag
	    if(includeVHlep){
	        //Add tighter cut on dr to tk
	        if(dataIs2011){
	            diphotonVHlep_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVHlepCut, subleadEtVHlepCut, 4, false, &smeared_pho_energy[0], true, true );
	            if(l.pho_drtotk_25_99[l.dipho_leadind[diphotonVHlep_id]] < 1 || l.pho_drtotk_25_99[l.dipho_subleadind[diphotonVHlep_id]] < 1) diphotonVHlep_id = -1;
	            VHmuevent=MuonTag2011(l, diphotonVHlep_id, &smeared_pho_energy[0]);
	            VHelevent=ElectronTag2011(l, diphotonVHlep_id, &smeared_pho_energy[0]);
	        } else {
	            //VHmuevent=MuonTag2012(l, diphotonVHlep_id, &smeared_pho_energy[0],lep_sync);
	            //VHelevent=ElectronTag2012(l, diphotonVHlep_id, &smeared_pho_energy[0],lep_sync);
	    	    float eventweight = weight * genLevWeight;
                VHmuevent=MuonTag2012B(l, diphotonVHlep_id, mu_ind, muVtx, VHmuevent_cat, &smeared_pho_energy[0], lep_sync, false, -0.2, eventweight, smeared_pho_weight, !isSyst);
                if(!VHmuevent){
                    VHelevent=ElectronTag2012B(l, diphotonVHlep_id, el_ind, elVtx, VHelevent_cat, &smeared_pho_energy[0], lep_sync, false, -0.2, eventweight, smeared_pho_weight, !isSyst);
                }
	        }
	    }

	//-------------------------------- ANIELLO --------------------------------//
	if(includeVHlepB ){
            float eventweight = weight * genLevWeight;
            float myweight=1.;
            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;
            VHmuevent_prov=MuonTag2012B(l,diphotonVHlep_id,mu_ind,muVtx,VHmuevent_cat,&smeared_pho_energy[0],lep_sync,false,-0.2,eventweight,smeared_pho_weight,!isSyst);
            int diphotonVH_ele_id=-1;
            VHelevent_prov=ElectronTag2012B(l,diphotonVH_ele_id,el_ind,elVtx,VHelevent_cat,&smeared_pho_energy[0],lep_sync,false,-0.2,eventweight,smeared_pho_weight,!isSyst);
            int diphotonVH_index_lep = -1;
	    int vertex = -1;
            if(VHmuevent_prov) {
                vertex=muVtx;
                diphotonVH_index_lep=diphotonVHlep_id;
            }
            if(!VHmuevent_prov && VHelevent_prov){
                vertex =elVtx;
                diphotonVH_index_lep=diphotonVH_ele_id;
                diphotonVHlep_id=diphotonVH_ele_id;
            } 
            if(VHmuevent_prov || VHelevent_prov){
		TLorentzVector lead_p4_metStudy = l.get_pho_p4( l.dipho_leadind[diphotonVH_index_lep], vertex, &smeared_pho_energy[0]);
                TLorentzVector sublead_p4_metStudy = l.get_pho_p4( l.dipho_subleadind[diphotonVH_index_lep], vertex, &smeared_pho_energy[0]);
                TLorentzVector MET;
                MET = l.METCorrection2012B(lead_p4_metStudy, sublead_p4_metStudy);

		int Njet_lepcat = 0; 
		for(int i=0; i<l.jet_algoPF1_n; i++){
		    TLorentzVector * p4_jet = (TLorentzVector *) l.jet_algoPF1_p4->At(i);
		    double dR_jet_PhoLead = p4_jet->DeltaR(lead_p4_metStudy);
		    double dR_jet_PhoSubLead = p4_jet->DeltaR(sublead_p4_metStudy);
		    double dR_jet_muon = 10.0;
		    double dR_jet_electron = 10.0;
		    if(VHelevent_prov){ 
			TLorentzVector* el_jet = (TLorentzVector*) l.el_std_p4->At(el_ind); 
			dR_jet_electron = p4_jet->DeltaR(*el_jet);
		    }
		    if(VHmuevent_prov){ 
			TLorentzVector* mu_jet = (TLorentzVector*) l.mu_glo_p4->At(mu_ind); 
			dR_jet_muon = p4_jet->DeltaR(*mu_jet);
		    }
		    if(dR_jet_PhoLead<0.5) continue;
		    if(dR_jet_PhoSubLead<0.5) continue;
		    if(dR_jet_electron<0.5) continue;
		    if(dR_jet_muon<0.5) continue;
		    if(p4_jet->Eta()>4.7) continue;
		    if(p4_jet->Pt()<20) continue;
		    Njet_lepcat = Njet_lepcat + 1;
		}
		
		if(Njet_lepcat<4){
		    if(MET.Pt()>=45) VHlep1event=true;
		    //-------------------------------- ANIELLO2 --------------------------------//
		    else{
			if(VHelevent_prov){
			    TLorentzVector* myelectron_M   = (TLorentzVector*) l.el_std_p4->At(el_ind);
			    TLorentzVector elpho1=*myelectron_M + lead_p4_metStudy;
			    TLorentzVector elpho2=*myelectron_M + sublead_p4_metStudy;
			    if(fabs(elpho1.M() - 91.19)>10 && fabs(elpho2.M() - 91.19)>10) VHlep2event=true;
			}		    
		    if(VHmuevent_prov) VHlep2event=true;
		    }
		//--------------------------------------------------------------------------//
		}
	    }
	}
	//-------------------------------------------------------------------------//


	    //Met tag //met at analysis step
        if(includeVHmet){
            int met_cat=-1;
            if( isSyst) VHmetevent=METTag2012B(l, diphotonVHmet_id, met_cat, &smeared_pho_energy[0], met_sync, false, -0.2, true);
            else        VHmetevent=METTag2012B(l, diphotonVHmet_id, met_cat, &smeared_pho_energy[0], met_sync, false, -0.2, false);
	    }

	    // VBF+hadronic VH
	    if((includeVBF || includeVHhad || includeVHhadBtag || includeTTHhad || includeTTHlep || includetHqLeptonic|| includeTprimehad || includeTprimelep || includeLoose)&&l.jet_algoPF1_n>1 && !isSyst /*avoid rescale > once*/) {
	        l.RescaleJetEnergy();cout<<"dentro rescale energy"<<endl;
	    }
	    cout<<"dopo rescale energy"<<endl;
	    if(includeVBF) {
	        diphotonVBF_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVBFCut, subleadEtVBFCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonVBF_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonVBF_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonVBF_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            VBFevent= ( dataIs2011 ?
	        	    VBFTag2011(l, diphotonVBF_id, &smeared_pho_energy[0], true, eventweight, myweight) :
	        	    VBFTag2012(vbfIjet1, vbfIjet2, l, diphotonVBF_id, &smeared_pho_energy[0], true, eventweight, myweight) )
	            ;
	        }
        }

        if(includeVHhad) {
	        diphotonVHhad_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVHhadCut, subleadEtVHhadCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonVHhad_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonVHhad_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonVHhad_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            VHhadevent = VHhadronicTag2011(l, diphotonVHhad_id, &smeared_pho_energy[0], true, eventweight, myweight);
	        }
	    }

        if(includeVHhadBtag) {
	        diphotonVHhadBtag_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtVHhadBtagCut, subleadEtVHhadBtagCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonVHhadBtag_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonVHhadBtag_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonVHhadBtag_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            VHhadBtagevent = VHhadronicBtag2012(l, diphotonVHhadBtag_id, &smeared_pho_energy[0], true, eventweight, myweight);
	        }
	    }




        if(includeTTHhad) {
	    cout<<"dentro tth had"<<endl;
	        diphotonTTHhad_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtTTHhadCut, subleadEtTTHhadCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonTTHhad_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonTTHhad_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonTTHhad_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            TTHhadevent = TTHhadronicTag2012(l, diphotonTTHhad_id, &smeared_pho_energy[0], true, eventweight, myweight);
	        }
	    }


        if(includeTTHlep) {
	    cout<<"dentro tth leo"<<endl;
	        diphotonTTHlep_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtTTHlepCut, subleadEtTTHlepCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotonTTHlep_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonTTHlep_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonTTHlep_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            TTHlepevent = TTHleptonicTag2012(l, diphotonTTHlep_id, &smeared_pho_energy[0], true, eventweight, myweight);

		    if(TTHlepevent){
			//			std::cout<<diphotonTTHlep_id<<" "<<l.diphoton_id_lep<<"||";
			if(l.diphoton_id_lep != -1)diphotonTTHlep_id=l.diphoton_id_lep;
		    }


	        }
	    }

        if(includetHqLeptonic) {
	        diphotontHqLeptonic_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEttHqLeptonicCut, subleadEttHqLeptonicCut, 4,false, &smeared_pho_energy[0], true);

            if(diphotontHqLeptonic_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotontHqLeptonic_id]] * smeared_pho_weight[l.dipho_subleadind[diphotontHqLeptonic_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;
		    //		    cout<<diphotontHqLeptonic_id<<endl;
	            tHqLeptonicevent = tHqLeptonicTag(l, diphotontHqLeptonic_id, &smeared_pho_energy[0], true, eventweight, myweight);
		    //		    cout<<l.diphoton_id_lep<<" "<<diphotontHqLeptonic_id<<endl;
		    if(tHqLeptonicevent) {
			//			cout<<l.diphoton_id_lep<<" "<<diphotontHqLeptonic_id<<endl;
			if(l.diphoton_id_lep != -1)diphotontHqLeptonic_id=l.diphoton_id_lep;
		    }
	        }
	    }


	/*GIUSEPPE*/
	if(includeLoose){
	    
	    int counter_photons_higgs=0;
	    for(int i=0;i<1919;i++){//sono salvate le prime 1919 particelle generate                                                                               
		if(l.gp_pdgid[i]==22 && l.gp_status[i]==3 && l.gp_pdgid[l.gp_mother[i]]==25){counter_photons_higgs++;}//un fotone che viene dall'Higgs                                     
	    }
	    diphotonLoose_id = l.DiphotonCiCSelection(l.phoNOCUTS, l.phoNOCUTS, 0, 0, 4,false, &smeared_pho_energy[0], true);//boh
	    //	        diphotonLoose_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, 0, 0, 4,false, &smeared_pho_energy[0], true);
	    if(counter_photons_higgs==2 && diphotonLoose_id!=-1){
		Looseevent = true; //LooseTag2012(l);
		//cout<<endl<<endl<<"evento LOOSE"<<endl;
		//if(diphotonLoose_id==-1){cout<<"DiphotonLoose_id<"<<diphotonLoose_id<<endl;}
	    }

	}

        if(includeTprimehad) {
	    cout<<"dentro Tprime had"<<endl;
	    int counter_photons_higgs=0;
	    /*	    for(int i=0;i<1919;i++){//sono salvate le prime 1919 particelle generate                                                                               
		if(l.gp_pdgid[i]==22 && l.gp_status[i]==3 && l.gp_pdgid[l.gp_mother[i]]==25){counter_photons_higgs++;}//un fotone che viene dall'Higgs                                     
		}*/
	        diphotonTprimehad_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtTprimehadCut, subleadEtTprimehadCut, 4,false, &smeared_pho_energy[0], true);

		if((counter_photons_higgs==2) && (diphotonTprimehad_id!=-1)){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonTprimehad_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonTprimehad_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            Tprimehadevent = TprimehadronicTag2012(l, diphotonTprimehad_id, &smeared_pho_energy[0], true, eventweight, myweight);
	        }
	}
    

        if(includeTprimelep) {
	    cout<<"dentro Tprime leo"<<endl;
	    int counter_photons_higgs=0;
	    /*	    for(int i=0;i<1919;i++){//sono salvate le prime 1919 particelle generate                                                                               
		if(l.gp_pdgid[i]==22 && l.gp_status[i]==3 && l.gp_pdgid[l.gp_mother[i]]==25){counter_photons_higgs++;}//un fotone che viene dall'Higgs                                     
		}*/
	        diphotonTprimelep_id = l.DiphotonCiCSelection(l.phoSUPERTIGHT, l.phoSUPERTIGHT, leadEtTprimelepCut, subleadEtTprimelepCut, 4,false, &smeared_pho_energy[0], true);

            if(counter_photons_higgs==2 && diphotonTprimelep_id!=-1){
	            float eventweight = weight * smeared_pho_weight[l.dipho_leadind[diphotonTprimelep_id]] * smeared_pho_weight[l.dipho_subleadind[diphotonTprimelep_id]] * genLevWeight;
	            float myweight=1.;
	            if(eventweight*sampleweight!=0) myweight=eventweight/sampleweight;

	            Tprimelepevent = TprimeleptonicTag2012(l, diphotonTprimelep_id, &smeared_pho_energy[0], true, eventweight, myweight);

		    if(Tprimelepevent){
			//			std::cout<<diphotonTprimelep_id<<" "<<l.diphoton_id_lep<<"||";
			if(l.diphoton_id_lep != -1)diphotonTprimelep_id=l.diphoton_id_lep;
		    }


	        }
	}
	/*GIUSEPPE*/

	cout<<"prima della priorita dell'analisi"<<endl;

	// priority of analysis:(Loose,)Tprime leptonic, Tprime hadronic, TTH leptonic, TTH hadronic, lepton tag, vbf, vhhad btag, vh had 0tag, vh met 
	if (includeLoose&&Looseevent) {//GIUSEPPE //La metto per prima, cosi' se la spengo la priorita' va a Tprime lep
	    diphoton_id = diphotonLoose_id;//GIUSEPPE
	} else if (includeTprimelep&&Tprimelepevent) {//GIUSEPPE
	    diphoton_id = diphotonTprimelep_id;//GIUSEPPE
	} else if(includeTprimehad&&Tprimehadevent) {//GIUSEPPE
	    cout<<endl<<endl<<"evento HADR"<<endl<<endl;
	    diphoton_id = diphotonTprimehad_id;//GIUSEPPE
	} else if (includetHqLeptonic&&tHqLeptonicevent) {
	    diphoton_id = diphotontHqLeptonic_id;
	} else  if (includeTTHlep&&TTHlepevent) {
	    diphoton_id = diphotonTTHlep_id;
	} else if(includeTTHhad&&TTHhadevent) {
	        diphoton_id = diphotonTTHhad_id;
	} else if(includeVHlep&&VHmuevent){
            diphoton_id = diphotonVHlep_id;
        } else if (includeVHlep&&VHelevent){
            diphoton_id = diphotonVHlep_id;
	} else if (includeVHlepB&&VHlep1event){ //ANIELLO
	    diphoton_id = diphotonVHlep_id;     //ANIELLO
	} else if (includeVHlepB&&VHlep2event){ //ANIELLO
	    diphoton_id = diphotonVHlep_id;     //ANIELLO
	}else if(includeVHhadBtag&&VHhadBtagevent) {
	    diphoton_id = diphotonVHhadBtag_id;
	} else if(includeVHhad&&VHhadevent) {
	    diphoton_id = diphotonVHhad_id;
	}else if(includeVBF&&VBFevent) {
	    diphoton_id = diphotonVBF_id;
	} else if(includeVHmet&&VHmetevent) {
	    diphoton_id = diphotonVHmet_id;
	} 
	// End exclusive mode selection
    }

    cout<<"DIPHOTON_ID"<<diphoton_id<<endl;
    //// std::cout << isSyst << " " << diphoton_id << " " << sumaccept << std::endl;

    // if we selected any di-photon, compute the Higgs candidate kinematics
    // and compute the event category
    if (diphoton_id > -1 ) {//LA CHIAVE DI TUTTO
        diphoton_index = std::make_pair( l.dipho_leadind[diphoton_id],  l.dipho_subleadind[diphoton_id] );

        // bring all the weights together: lumi & Xsection, single gammas, pt kfactor
	evweight = weight * smeared_pho_weight[diphoton_index.first] * smeared_pho_weight[diphoton_index.second] * genLevWeight;
	if( ! isSyst ) {
	    l.countersred[diPhoCounter_]++;
	}
	cout<<"prima dei TLorentz"<<endl;
        TLorentzVector lead_p4, sublead_p4, Higgs;//qui si definisce il vettore Higgs
        float lead_r9, sublead_r9;
        TVector3 * vtx;
	if(includeVHlep){  //ANIELLO
        if(( (includeVHlep && (VHelevent || VHmuevent))) && !(includeVBF&&VBFevent) ) {
            if(VHmuevent){
		cout<<"vedi"<<endl;
	            fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, l.dipho_leadind[diphoton_id],  l.dipho_subleadind[diphoton_id], 0);  // use default vertex for the muon tag
            } else if(VHelevent){
		cout<<"vedi2"<<endl;
                if(nElectronCategories==2){
                    if(PADEBUG) std::cout<<"nElectronCategories "<<nElectronCategories<<std::endl;
	                fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, leadpho_ind, subleadpho_ind, elVtx);  // use elVtx for ElectronTag2012B
                    if(PADEBUG) std::cout<<"post fillDiphoton Higgs.Pt() "<<Higgs.Pt()<<std::endl;
                } else {
		cout<<"vedi3"<<endl;
                    fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, l.dipho_leadind[diphoton_id],  l.dipho_subleadind[diphoton_id], 0);  // use default vertex for old electron tag
                }
            }
        } else {
	    cout<<"vedi4"<<endl;//infatti entra qui!!!! e quando non sono accese non sa cosa fare
	        fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);
        }
	} //ANIELLO//chiude include VHlep
	//-------------------------------- ANIELLO --------------------------------//
	if(includeVHlepB){
	    if(( (includeVHlepB && (VHlep1event || VHlep2event))) && !(includeVBF&&VBFevent) ) {
		if(VHlep1event){
		cout<<"vedi5"<<endl;
	            fillDiphoton(lead_p4,sublead_p4,Higgs,lead_r9,sublead_r9,vtx,&smeared_pho_energy[0],l,l.dipho_leadind[diphoton_id],l.dipho_subleadind[diphoton_id],0); 
		} else if(VHlep2event){
		cout<<"vedi6"<<endl;
		    fillDiphoton(lead_p4,sublead_p4,Higgs,lead_r9,sublead_r9,vtx,&smeared_pho_energy[0],l,l.dipho_leadind[diphoton_id],l.dipho_subleadind[diphoton_id],0); 
		}
	    } else {
		cout<<"vedi7"<<endl;
	        fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);
	    }
	}//chiude includeVHlepB
	cout<<"Prima di fillDiphoton"<<endl;
       	if(includeLoose && Looseevent){fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);}//NON SO COSA HO FATTO!!!!}
       	if(includeTTHlep && TTHlepevent){fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);}//NON SO COSA HO FATTO!!!!}
	if(includeTTHhad && TTHhadevent){fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);}//NON SO COSA HO FATTO!!!!}
if(includeTprimelep && Tprimelepevent){fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);}//NON SO COSA HO FATTO!!!!}
	if(includeTprimehad && Tprimehadevent){fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);}//NON SO COSA HO FATTO!!!!}

	cout<<"prima di beamspot"<<endl;
        // apply beamspot reweighting if necessary
        if(reweighBeamspot && cur_type!=0) {
	    cout<<"dento reweightBeamspot"<<endl;
	    fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);//NOTA BENE::l'ho messo io, per evitare che crasha per il MC	  
  //cout<<"dentro if reweightBeamSpot"<<endl;
	    //cout<<"ecco"<<BeamspotReweight(vtx->Z(),((TVector3*)l.gv_pos->At(0))->Z())<<endl;
	    // cout<<"qui e\ il problema"<<endl;
            evweight*=BeamspotReweight(vtx->Z(),((TVector3*)l.gv_pos->At(0))->Z());
        }

        // FIXME pass smeared R9
	cout<<"prima di diphoton"<<endl;
        category = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,nPtCategories,nVtxCategories,l.vtx_std_n);
        mass     = Higgs.M();
	cout<<"prima di di_photon level"<<endl;
        // apply di-photon level smearings and corrections
        int selectioncategory = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,0,nVtxCategories,l.vtx_std_n);
        if( cur_type != 0 && doMCSmearing ) {
	    applyDiPhotonSmearings(Higgs, *vtx, selectioncategory, cur_type, *((TVector3*)l.gv_pos->At(0)), evweight, zero_, zero_,
				   diPhoSys, syst_shift);
            isCorrectVertex=(*vtx- *((TVector3*)l.gv_pos->At(0))).Mag() < 1.;
        }

        float ptHiggs = Higgs.Pt();
        //if (cur_type != 0) cout << "vtxAn: " << isCorrectVertex << endl;
	// sanity check
        assert( evweight >= 0. );

	// see if the event falls into an exclusive category
	cout<<"prima di computeExclusiceCat"<<endl;
	computeExclusiveCategory(l, category, diphoton_index, Higgs.Pt() );
	cout<<"dopo computeExc"<<endl;
	cout<<"Dove cazzo vanno: ?category"<<category<<endl;
	cout<<"CHE CAZZO DI SCHIFO: Massa Higgs"<<Higgs.M()<<endl;
	cout<<"ptHiggs"<<ptHiggs<<endl;
	// fill control plots and counters
	cout<<"isSyst"<<isSyst<<endl;
	cout<<"Cerca bene qui"<<endl;
	if( ! isSyst ) {
	    cout<<"dentro !isSyst"<<endl;
	    l.FillCounter( "Accepted", weight );
	    l.FillCounter( "Smeared", evweight );
	    sumaccept += weight;
	    sumsmear += evweight;
	    cout<<"Prima di FillCounter"<<endl;
	    //fillControlPlots(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, diphoton_id,
	    //	     category, isCorrectVertex, evweight, vtx, l, muVtx, mu_ind, elVtx, el_ind );
        
	    cout<<"prima di fill opt tree"<<endl;
	    cout<<"fillopt"<<fillOptTree<<endl;
	//fill opt tree
	if (fillOptTree) {
	    //qui dentro e il problema maledetto!!! GIUSEPPE'S CHASE
	    //	    cout<<"diphoton_index"<<diphoton_index<<endl;
	    cout<<"weight"<<weight<<endl;
	    cout<<"evweight"<<evweight<<endl;
	    cout<<"mass"<<mass<<endl;
	    cout<<"Higgs?"<<endl;
	    cout<<"category"<<category<<endl;
	    cout<<"VBFevent"<<VBFevent<<endl;
	    cout<<"myBF_Mjj"<<myVBF_Mjj<<endl;
	    cout<<"myVBFLead"<<myVBFLeadJPt<<endl;
	    cout<<"myVBFSubJPt"<<myVBFSubJPt<<endl;
	    cout<<"nVBFDijetJetCategories"<<nVBFDijetJetCategories<<endl;

	    fillOpTree(l, lead_p4, sublead_p4, &smeared_pho_energy[0], -2, diphoton_index, diphoton_id, -2, -2, weight, evweight,
		       mass, -1, -1, Higgs, -2, category, VBFevent, myVBF_Mjj, myVBFLeadJPt,
		       myVBFSubJPt, nVBFDijetJetCategories, isSyst, "no-syst");
	}
	}
	cout<<"dopo fill opt tree"<<endl;
        // dump BS trees if requested
        if (!isSyst && cur_type!=0 && saveBSTrees_) saveBSTrees(l, evweight,category,Higgs, vtx, (TVector3*)l.gv_pos->At(0));

        // save trees for unbinned datacards
        int inc_cat = l.DiphotonCategory(diphoton_index.first,diphoton_index.second,Higgs.Pt(),nEtaCategories,nR9Categories,R9CatBoundary,nPtCategories,nVtxCategories,l.vtx_std_n);
        if (!isSyst && cur_type<0 && saveDatacardTrees_) saveDatCardTree(l,cur_type,category, inc_cat, evweight, diphoton_index.first,diphoton_index.second,l.dipho_vtxind[diphoton_id],lead_p4,sublead_p4,true);

	cout<<"prima di vbf trees"<<endl;
	//save vbf trees
	if (!isSyst && cur_type<0 && saveVBFTrees_) saveVBFTree(l,category, evweight, -99.);


        if (dumpAscii && !isSyst && (cur_type==0||dumpMcAscii) && mass>=massMin && mass<=massMax ) {
            // New ascii event list for syncrhonizing MVA Preselection + Diphoton MVA
            eventListText <<"type:"<< cur_type 
              << "    run:"   <<  l.run
              << "    lumi:"  <<  l.lumis
              << "    event:" <<  l.event
              << "    rho:" <<    l.rho
	      << "    nvtx:" << l.vtx_std_n
	      << "    weight:" << evweight
            // Preselection Lead
              << "    r9_1:"  <<  lead_r9
              << "    sceta_1:"   << (photonInfoCollection[diphoton_index.first]).caloPosition().Eta() 
              << "    hoe_1:" <<  l.pho_hoe[diphoton_index.first]
              << "    sigieie_1:" <<  l.pho_sieie[diphoton_index.first]
              << "    ecaliso_1:" <<  l.pho_ecalsumetconedr03[diphoton_index.first] - 0.012*lead_p4.Et()
              << "    hcaliso_1:" <<  l.pho_hcalsumetconedr03[diphoton_index.first] - 0.005*lead_p4.Et()
              << "    trckiso_1:" <<  l.pho_trksumpthollowconedr03[diphoton_index.first] - 0.002*lead_p4.Et()
              << "    chpfiso2_1:" <<  (*l.pho_pfiso_mycharged02)[diphoton_index.first][l.dipho_vtxind[diphoton_id]] 
              << "    chpfiso3_1:" <<  (*l.pho_pfiso_mycharged03)[diphoton_index.first][l.dipho_vtxind[diphoton_id]] 
            // Preselection SubLead
              << "    r9_2:"  <<  sublead_r9
              << "    sceta_2:"   << (photonInfoCollection[diphoton_index.second]).caloPosition().Eta() 
              << "    hoe_2:" <<  l.pho_hoe[diphoton_index.second]
              << "    sigieie_2:" <<  l.pho_sieie[diphoton_index.second]
              << "    ecaliso_2:" <<  l.pho_ecalsumetconedr03[diphoton_index.second] - 0.012*sublead_p4.Et()
              << "    hcaliso_2:" <<  l.pho_hcalsumetconedr03[diphoton_index.second] - 0.005*sublead_p4.Et()
              << "    trckiso_2:" <<  l.pho_trksumpthollowconedr03[diphoton_index.second] - 0.002*sublead_p4.Et()
              << "    chpfiso2_2:" <<  (*l.pho_pfiso_mycharged02)[diphoton_index.second][l.dipho_vtxind[diphoton_id]] 
              << "    chpfiso3_2:" <<  (*l.pho_pfiso_mycharged03)[diphoton_index.second][l.dipho_vtxind[diphoton_id]] 
        // Diphoton MVA inputs
              << "    ptH:"  <<  ptHiggs 
              << "    phoeta_1:"  <<  lead_p4.Eta() 
              << "    phoeta_2:"  <<  sublead_p4.Eta() 
              << "    bsw:"       <<  beamspotWidth 
              << "    pt_1:"      <<  lead_p4.Pt()
              << "    pt_2:"      <<  sublead_p4.Pt()
              << "    pt_1/m:"      <<  lead_p4.Pt()/mass
              << "    pt_2/m:"      <<  sublead_p4.Pt()/mass
              << "    cosdphi:"   <<  TMath::Cos(lead_p4.Phi() - sublead_p4.Phi()) 
        // Extra
              << "    mgg:"       <<  mass 
              << "    e_1:"       <<  lead_p4.E()
              << "    e_2:"       <<  sublead_p4.E()
              << "    vbfevent:"  <<  VBFevent
              << "    muontag:"   <<  VHmuevent
              << "    electrontag:"<< VHelevent
              << "    mettag:"    <<  VHmetevent
              << "    evcat:"     <<  category
              << "    FileName:"  <<  l.files[l.current]
            // VBF MVA
              << "    vbfmva: "   <<  myVBF_MVA;
            // Vertex MVA
            vtxAna_.setPairID(diphoton_id);
            std::vector<int> & vtxlist = l.vtx_std_ranked_list->at(diphoton_id);
            // Conversions
            PhotonInfo p1 = l.fillPhotonInfos(l.dipho_leadind[diphoton_id], vtxAlgoParams.useAllConversions, 0); // WARNING using default photon energy: it's ok because we only re-do th$
            PhotonInfo p2 = l.fillPhotonInfos(l.dipho_subleadind[diphoton_id], vtxAlgoParams.useAllConversions, 0); // WARNING using default photon energy: it's ok because we only re-do$
            int convindex1 = l.matchPhotonToConversion(diphoton_index.first,vtxAlgoParams.useAllConversions);
            int convindex2 = l.matchPhotonToConversion(diphoton_index.second,vtxAlgoParams.useAllConversions);

            for(size_t ii=0; ii<3; ++ii ) {
                eventListText << "\tvertexId"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxlist[ii] : -1);
            }
            for(size_t ii=0; ii<3; ++ii ) {
                eventListText << "\tvertexMva"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxAna_.mva(vtxlist[ii]) : -2.);
            }
            for(size_t ii=1; ii<3; ++ii ) {
                eventListText << "\tvertexdeltaz"<< ii+1 <<":" << (ii < vtxlist.size() ? vtxAna_.vertexz(vtxlist[ii])-vtxAna_.vertexz(vtxlist[0]) : -999.);
            }
            eventListText << "\tptbal:"   << vtxAna_.ptbal(vtxlist[0])
                          << "\tptasym:"  << vtxAna_.ptasym(vtxlist[0])
                          << "\tlogspt2:" << vtxAna_.logsumpt2(vtxlist[0])
                          << "\tp2conv:"  << vtxAna_.pulltoconv(vtxlist[0])
                          << "\tnconv:"   << vtxAna_.nconv(vtxlist[0]);
	cout<<"prima di Photon IDMVA inputs"<<endl;
        //Photon IDMVA inputs
            double pfchargedisobad03=0.;
            for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++) {
                pfchargedisobad03 = l.pho_pfiso_mycharged03->at(diphoton_index.first).at(ivtx) > pfchargedisobad03 ? l.pho_pfiso_mycharged03->at(diphoton_index.first).at(ivtx) : pfchargedisobad03;
            }

            eventListText << "\tscetawidth_1: " << l.pho_etawidth[diphoton_index.first]
                          << "\tscphiwidth_1: " << l.sc_sphi[diphoton_index.first]
                          << "\tsieip_1: " << l.pho_sieip[diphoton_index.first]
                          << "\tbc_e2x2_1: " << l.pho_e2x2[diphoton_index.first]
                          << "\tpho_e5x5_1: " << l.bc_s25[l.sc_bcseedind[l.pho_scind[diphoton_index.first]]]
                          << "\ts4ratio_1: " << l.pho_s4ratio[diphoton_index.first]
                          << "\tpfphotoniso03_1: " << l.pho_pfiso_myphoton03[diphoton_index.first]
                          << "\tpfchargedisogood03_1: " << l.pho_pfiso_mycharged03->at(diphoton_index.first).at(vtxlist[0])
                          << "\tpfchargedisobad03_1: " << pfchargedisobad03
                          << "\teseffsigmarr_1: " << l.pho_ESEffSigmaRR[diphoton_index.first];
            pfchargedisobad03=0.;
            for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++) {
                pfchargedisobad03 = l.pho_pfiso_mycharged03->at(diphoton_index.second).at(ivtx) > pfchargedisobad03 ? l.pho_pfiso_mycharged03->at(diphoton_index.second).at(ivtx) : pfchargedisobad03;
            }

            eventListText << "\tscetawidth_2: " << l.pho_etawidth[diphoton_index.second]
                          << "\tscphiwidth_2: " << l.sc_sphi[diphoton_index.second]
                          << "\tsieip_2: " << l.pho_sieip[diphoton_index.second]
                          << "\tbc_e2x2_2: " << l.pho_e2x2[diphoton_index.second]
                          << "\tpho_e5x5_2: " << l.bc_s25[l.sc_bcseedind[l.pho_scind[diphoton_index.second]]]
                          << "\ts4ratio_2: " << l.pho_s4ratio[diphoton_index.second]
                          << "\tpfphotoniso03_2: " << l.pho_pfiso_myphoton03[diphoton_index.second]
                          << "\tpfchargedisogood03_2: " << l.pho_pfiso_mycharged03->at(diphoton_index.second).at(vtxlist[0])
                          << "\tpfchargedisobad03_2: " << pfchargedisobad03
                          << "\teseffsigmarr_2: " << l.pho_ESEffSigmaRR[diphoton_index.second];


            if (convindex1!=-1) {
                eventListText 
                << "    convVtxZ1:"  <<  vtxAna_.vtxZFromConv(p1)
                << "    convVtxdZ1:"  <<  vtxAna_.vtxZFromConv(p1)-vtxAna_.vertexz(vtxlist[0])
                << "    convRes1:"   << vtxAna_.vtxdZFromConv(p1) 
                << "    convChiProb1:"  <<  l.conv_chi2_probability[convindex1]
                << "    convNtrk1:"  <<  l.conv_ntracks[convindex1]
                << "    convindex1:"  <<  convindex1
                << "    convvtxZ1:" << ((TVector3*) l.conv_vtx->At(convindex1))->Z()
                << "    convvtxR1:" << ((TVector3*) l.conv_vtx->At(convindex1))->Perp()
                << "    convrefittedPt1:" << ((TVector3*) l.conv_refitted_momentum->At(convindex1))->Pt();
            } else {
                eventListText 
                << "    convVtxZ1:"  <<  -999
                << "    convVtxdZ1:"  <<  -999
                << "    convRes1:"    <<  -999
                << "    convChiProb1:"  <<  -999
                << "    convNtrk1:"  <<  -999
                << "    convindex1:"  <<  -999
                << "    convvtxZ1:" << -999
                << "    convvtxR1:" << -999
                << "    convrefittedPt1:" << -999;
            }
            if (convindex2!=-1) {
                eventListText 
                << "    convVtxZ2:"  <<  vtxAna_.vtxZFromConv(p2)
                << "    convVtxdZ2:"  <<  vtxAna_.vtxZFromConv(p2)-vtxAna_.vertexz(vtxlist[0])
                << "    convRes2:"   << vtxAna_.vtxdZFromConv(p2)
                << "    convChiProb2:"  <<  l.conv_chi2_probability[convindex2]
                << "    convNtrk2:"  <<  l.conv_ntracks[convindex2]
                << "    convindex2:"  <<  convindex2
                << "    convvtxZ2:" << ((TVector3*) l.conv_vtx->At(convindex2))->Z()
                << "    convvtxR2:" << ((TVector3*) l.conv_vtx->At(convindex2))->Perp()
                << "    convrefittedPt2:" << ((TVector3*) l.conv_refitted_momentum->At(convindex2))->Pt();
            } else {
                eventListText 
                << "    convVtxZ2:"  <<  -999
                << "    convVtxdZ2:"  <<  -999
                << "    convRes2:"    <<  -999
                << "    convChiProb2:"  <<  -999
                << "    convNtrk2:"  <<  -999
                << "    convindex2:"  <<  -999
                << "    convvtxZ2:" << -999
                << "    convvtxR2:" << -999
                << "    convrefittedPt2:" << -999;
            }
            	cout<<"prima di VHelevent"<<endl;
            if(VHelevent){
                TLorentzVector* myel = (TLorentzVector*) l.el_std_p4->At(el_ind);
                TLorentzVector* myelsc = (TLorentzVector*) l.el_std_sc->At(el_ind);
                float thiseta = fabs(myelsc->Eta());
                double Aeff=0.;
                if(thiseta<1.0)                   Aeff=0.135;
                if(thiseta>=1.0 && thiseta<1.479) Aeff=0.168;
                if(thiseta>=1.479 && thiseta<2.0) Aeff=0.068;
                if(thiseta>=2.0 && thiseta<2.2)   Aeff=0.116;
                if(thiseta>=2.2 && thiseta<2.3)   Aeff=0.162;
                if(thiseta>=2.3 && thiseta<2.4)   Aeff=0.241;
                if(thiseta>=2.4)                  Aeff=0.23;
                float thisiso=l.el_std_pfiso_charged[el_ind]+std::max(l.el_std_pfiso_neutral[el_ind]+l.el_std_pfiso_photon[el_ind]-l.rho*Aeff,0.);
    	cout<<"prima di elpho1"<<endl;
                TLorentzVector elpho1=*myel + lead_p4;
                TLorentzVector elpho2=*myel + sublead_p4;

                eventListText 
                    << "    elind:"<<       el_ind
                    << "    elpt:"<<        myel->Pt()
                    << "    eleta:"<<       myel->Eta()
                    << "    elsceta:"<<     myelsc->Eta()
                    << "    elmva:"<<       l.el_std_mva_nontrig[el_ind]
                    << "    eliso:"<<       thisiso
                    << "    elisoopt:"<<    thisiso/myel->Pt()
                    << "    elAeff:"<<      Aeff
                    << "    eldO:"<<        fabs(l.el_std_D0Vtx[el_ind][elVtx])
                    << "    eldZ:"<<        fabs(l.el_std_DZVtx[el_ind][elVtx])
                    << "    elmishits:"<<   l.el_std_hp_expin[el_ind]
                    << "    elconv:"<<      l.el_std_conv[el_ind]
                    << "    eldr1:"<<       myel->DeltaR(lead_p4)
                    << "    eldr2:"<<       myel->DeltaR(sublead_p4)
                    << "    elmeg1:"<<      elpho1.M()
                    << "    elmeg2:"<<      elpho2.M();

            } else {
                eventListText 
                    << "    elind:"<<       -1
                    << "    elpt:"<<        -1
                    << "    eleta:"<<       -1
                    << "    elsceta:"<<     -1
                    << "    elmva:"<<       -1
                    << "    eliso:"<<       -1
                    << "    elisoopt:"<<    -1
                    << "    elAeff:"<<      -1
                    << "    eldO:"<<        -1
                    << "    eldZ:"<<        -1
                    << "    elmishits:"<<   -1
                    << "    elconv:"<<      -1
                    << "    eldr1:"<<       -1
                    << "    eldr2:"<<       -1
                    << "    elmeg1:"<<      -1
                    << "    elmeg2:"<<      -1;
            }

            if(VHmetevent){
                TLorentzVector myMet = l.METCorrection2012B(lead_p4, sublead_p4); 
                float corrMet    = myMet.Pt();
                float corrMetPhi = myMet.Phi();

                eventListText 
                    << "    metuncor:"<<        l.met_pfmet
                    << "    metphiuncor:"<<     l.met_phi_pfmet
                    << "    metcor:"<<          corrMet
                    << "    metphicor:"<<       corrMetPhi;
            } else {
                eventListText 
                    << "    metuncor:"<<        -1
                    << "    metphiuncor:"<<     -1
                    << "    metcor:"<<          -1
                    << "    metphicor:"<<       -1;
            }
        
	        if( VBFevent ) {
		        eventListText 
			      << "\tjetPt1:"  << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1) )->Pt()
			      << "\tjetPt2:"  << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2) )->Pt()
			      << "\tjetEta1:" << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1) )->Eta()
			      << "\tjetEta2:" << ( (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2) )->Eta()
		        ;
		        dumpJet(eventListText,1,l,vbfIjet1);
		        dumpJet(eventListText,2,l,vbfIjet2);
	        }

            eventListText << endl;
        }
	cout<<"prima di chiedere a Francesco"<<endl;
	evweight*=5;
	//CHIEDERE A FRANCESCO!!!!
	//systematics for TTH??GIUSEPPE AND FOR TPRIME?? NON CAPISCO COME e` categorizzato....boh
	//switch on and off with applyBtagSF and appplyLeptonSF

	//shifting btag eff for efficiency
	//	computeBtagEff(l);
	//	if(jentry>(l.Entries_-20))	    std::cout<<" nBtagB: "<<nBtagB<<" nBMC:"<<nBMC<<" nBtagC: "<<nBtagC<<" nCMC:"<<nCMC<<" nBtagL: "<<nBtagL<<" nLMC:"<<nLMC<<endl;
	bool isMC = l.itype[l.current]!=0;
	if(includeTTHlep){
	    if(isMC && applyBtagSF && (category==4 ||category ==5 ||category==6 ||category==7)){//era 6 e 7 //Metto le mani anche qui!!!
		//	    cout<<"ev"<<evweight<<endl;
		evweight*=BtagReweight(l,shiftBtagEffUp_bc,shiftBtagEffDown_bc,shiftBtagEffUp_l,shiftBtagEffDown_l);
		//	    	        cout<<"evrew"<<evweight<<endl;
	    }//shiftbtag end
	}else if(includeVHhad){
	    if(isMC && applyBtagSF && category==6 ){//non toccato, e sempre stato 6
		//	    cout<<"ev"<<evweight<<endl;
		evweight*=BtagReweight(l,shiftBtagEffUp_bc,shiftBtagEffDown_bc,shiftBtagEffUp_l,shiftBtagEffDown_l);
		//	    	        cout<<"evrew"<<evweight<<endl;
	    }//shiftbtag end

	}


	if(isMC && applyLeptonSF && (category == 4||category==6)){//era 6 //metto le mani anche qui
	    //	    cout<<evweight;
	    if(l.isLep_ele) evweight*=ElectronSFReweight(l);
	    if(l.isLep_mu)  evweight*=MuonSFReweight(l);
	    //	    cout<<" weighted "<<evweight<<endl;
	//cout<<"runNumber: "<<l.run<<" LumiSection:"<<l.lumis<<" eventNumber:"<<l.event<<endl; 
	}

	cout<<"Questa puo essere la soluzione"<<endl;
        return (category >= 0 && mass>=massMin && mass<=massMax);
    
    } //chiude if (diphoton_id > -1 ) {
    cout<<"alla fine di Analyse Events di StatAnalysis"<<endl;
    return false;//termina AnalyseEvents
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::FillRooContainer(LoopAll& l, int cur_type, float mass, float diphotonMVA,
				    int category, float weight, bool isCorrectVertex, int diphoton_id)
{
    // Fill full mva trees
    if (doFullMvaFinalTree){
        int lead_ind = l.dipho_leadind[diphoton_id];
        int sublead_ind = l.dipho_subleadind[diphoton_id];
        if (PADEBUG) cout << "---------------" << endl;
        if (PADEBUG) cout << "Filling nominal vals" << endl;
        l.FillTree("mass",mass,"full_mva_trees");
        l.FillTree("bdtoutput",diphotonMVA,"full_mva_trees");
        l.FillTree("category",category,"full_mva_trees");
        l.FillTree("weight",weight,"full_mva_trees");
        l.FillTree("lead_r9",l.pho_r9[lead_ind],"full_mva_trees");
        l.FillTree("sublead_r9",l.pho_r9[sublead_ind],"full_mva_trees");
        l.FillTree("lead_eta",((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
        l.FillTree("sublead_eta",((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
        l.FillTree("lead_phi",((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
        l.FillTree("sublead_phi",((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
    }

    if (cur_type == 0 ) {
        l.rooContainer->InputDataPoint("data_mass",category,mass);
    } else if (cur_type > 0 ) {
        if( doMcOptimization ) {
            l.rooContainer->InputDataPoint("data_mass",category,mass,weight);
        } else {
            l.rooContainer->InputDataPoint("bkg_mass",category,mass,weight);
        }
    } else if (cur_type < 0) {
        l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l),category,mass,weight);
        if (isCorrectVertex) l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l)+"_rv",category,mass,weight);
        else l.rooContainer->InputDataPoint("sig_"+GetSignalLabel(cur_type, l)+"_wv",category,mass,weight);
    }
    //if( category>=0 && fillOptTree ) {
	//l.FillTree("run",l.run);
	//l.FillTree("lumis",l.lumis);
	//l.FillTree("event",l.event);
	//l.FillTree("category",category);
	//l.FillTree("vbfMVA",myVBF_MVA);
	//l.FillTree("vbfMVA0",myVBF_MVA0);
	//l.FillTree("vbfMVA1",myVBF_MVA1);
	//l.FillTree("vbfMVA2",myVBF_MVA2);
	///// l.FillTree("VBFevent", VBFevent);
	//if( vbfIjet1 > -1 && vbfIjet2 > -1 && ( myVBF_MVA > -2. ||  myVBF_MVA0 > -2 || myVBF_MVA1 > -2 || myVBF_MVA2 > -2 || VBFevent ) ) {
	//    TLorentzVector* jet1 = (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet1);
	//    TLorentzVector* jet2 = (TLorentzVector*)l.jet_algoPF1_p4->At(vbfIjet2);

	//    l.FillTree("leadJPt", myVBFLeadJPt);
	//    l.FillTree("subleadJPt", myVBFSubJPt);
	//    l.FillTree("leadJEta", (float)jet1->Eta());
	//    l.FillTree("subleadJEta", (float)jet2->Eta());
	//    l.FillTree("leadJPhi", (float)jet1->Phi());
	//    l.FillTree("subleadJPhi", (float)jet2->Phi());
	//    l.FillTree("MJJ", myVBF_Mjj);
	//    l.FillTree("deltaEtaJJ", myVBFdEta);
	//    l.FillTree("Zep", myVBFZep);
	//    l.FillTree("deltaPhiJJGamGam", myVBFdPhi);
	//    l.FillTree("MGamGam", myVBF_Mgg);
	//    l.FillTree("diphoPtOverM", myVBFDiPhoPtOverM);
	//    l.FillTree("leadPtOverM", myVBFLeadPhoPtOverM);
	//    l.FillTree("subleadPtOverM", myVBFSubPhoPtOverM);
	//    l.FillTree("leadJEta",myVBF_leadEta);
	//    l.FillTree("subleadJEta",myVBF_subleadEta);
	//    l.FillTree("VBF_Pz", myVBF_Pz);
	//    l.FillTree("VBF_S", myVBF_S);
	//    l.FillTree("VBF_K1", myVBF_K1);
	//    l.FillTree("VBF_K2", myVBF_K2);

	//    l.FillTree("deltaPhiGamGam", myVBF_deltaPhiGamGam);
	//    l.FillTree("etaJJ", myVBF_etaJJ);
	//    l.FillTree("VBFSpin_Discriminant", myVBFSpin_Discriminant);
	//    l.FillTree("deltaPhiJJ",myVBFSpin_DeltaPhiJJ);
	//    l.FillTree("cosThetaJ1", myVBFSpin_CosThetaJ1);
	//    l.FillTree("cosThetaJ2", myVBFSpin_CosThetaJ2);
	//    // Small deflection
	//    l.FillTree("deltaPhiJJS",myVBFSpin_DeltaPhiJJS);
	//    l.FillTree("cosThetaS", myVBFSpin_CosThetaS);
	//    // Large deflection
	//    l.FillTree("deltaPhiJJL",myVBFSpin_DeltaPhiJJL);
	//    l.FillTree("cosThetaL", myVBFSpin_CosThetaL);
	//}
	//l.FillTree("sampleType",cur_type);
	////// l.FillTree("isCorrectVertex",isCorrectVertex);
	////// l.FillTree("metTag",VHmetevent);
	////// l.FillTree("eleTag",VHelevent);
	////// l.FillTree("muTag",VHmuevent);

	//TLorentzVector lead_p4, sublead_p4, Higgs;
	//float lead_r9, sublead_r9;
	//TVector3 * vtx;
	//fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id);
	//l.FillTree("leadPt",(float)lead_p4.Pt());
	//l.FillTree("subleadPt",(float)sublead_p4.Pt());
	//l.FillTree("leadEta",(float)lead_p4.Eta());
	//l.FillTree("subleadEta",(float)sublead_p4.Eta());
	//l.FillTree("leadPhi",(float)lead_p4.Phi());
	//l.FillTree("subleadPhi",(float)sublead_p4.Phi());
	//l.FillTree("leadR9",lead_r9);
	//l.FillTree("subleadR9",sublead_r9);
	//l.FillTree("sigmaMrv",sigmaMrv);
	//l.FillTree("sigmaMwv",sigmaMwv);
	//l.FillTree("leadPhoEta",(float)lead_p4.Eta());
	//l.FillTree("subleadPhoEta",(float)sublead_p4.Eta());


	//vtxAna_.setPairID(diphoton_id);
	//float vtxProb = vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n);
	//float altMass = 0.;
	//if( l.vtx_std_n > 1 ) {
	//    int altvtx = (*l.vtx_std_ranked_list)[diphoton_id][1];
	//    altMass = ( l.get_pho_p4( l.dipho_leadind[diphoton_id], altvtx, &smeared_pho_energy[0]) +
	//		l.get_pho_p4( l.dipho_subleadind[diphoton_id], altvtx, &smeared_pho_energy[0]) ).M();
	//}
	//l.FillTree("altMass",altMass);
	//l.FillTree("vtxProb",vtxProb);
    //}
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::AccumulateSyst(int cur_type, float mass, float diphotonMVA,
				  int category, float weight,
				  std::vector<double> & mass_errors,
				  std::vector<double> & mva_errors,
				  std::vector<int>    & categories,
				  std::vector<double> & weights)
{
    categories.push_back(category);
    mass_errors.push_back(mass);
    mva_errors.push_back(diphotonMVA);
    weights.push_back(weight);
}


// ----------------------------------------------------------------------------------------------------
void StatAnalysis::FillRooContainerSyst(LoopAll& l, const std::string &name, int cur_type,
					std::vector<double> & mass_errors, std::vector<double> & mva_errors,
					std::vector<int>    & categories, std::vector<double> & weights, int diphoton_id)
{
    if (cur_type < 0){
	// fill full mva trees
	if (doFullMvaFinalTree){
	    assert(mass_errors.size()==2 && mva_errors.size()==2 && weights.size()==2 && categories.size()==2);
        int lead_ind = l.dipho_leadind[diphoton_id];
        int sublead_ind = l.dipho_subleadind[diphoton_id];
	    if (PADEBUG) cout << "Filling template models " << name << endl;
	    l.FillTree(Form("mass_%s_Down",name.c_str()),mass_errors[0],"full_mva_trees");
	    l.FillTree(Form("mass_%s_Up",name.c_str()),mass_errors[1],"full_mva_trees");
	    l.FillTree(Form("bdtoutput_%s_Down",name.c_str()),mva_errors[0],"full_mva_trees");
	    l.FillTree(Form("bdtoutput_%s_Up",name.c_str()),mva_errors[1],"full_mva_trees");
	    l.FillTree(Form("weight_%s_Down",name.c_str()),weights[0],"full_mva_trees");
	    l.FillTree(Form("weight_%s_Up",name.c_str()),weights[1],"full_mva_trees");
	    l.FillTree(Form("category_%s_Down",name.c_str()),categories[0],"full_mva_trees");
	    l.FillTree(Form("category_%s_Up",name.c_str()),categories[1],"full_mva_trees");
        /*
        l.FillTree(Form("lead_r9_%s_Down",name.c_str()),l.pho_r9[lead_ind],"full_mva_trees");
        l.FillTree(Form("lead_r9_%s_Up",name.c_str()),l.pho_r9[lead_ind],"full_mva_trees");
        l.FillTree(Form("sublead_r9_%s_Down",name.c_str()),l.pho_r9[sublead_ind],"full_mva_trees");
        l.FillTree(Form("sublead_r9_%s_Up",name.c_str()),l.pho_r9[sublead_ind],"full_mva_trees");
        l.FillTree(Form("lead_eta_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
        l.FillTree(Form("lead_eta_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Eta(),"full_mva_trees");
        l.FillTree(Form("sublead_eta_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
        l.FillTree(Form("sublead_eta_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Eta(),"full_mva_trees");
        l.FillTree(Form("lead_phi_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
        l.FillTree(Form("lead_phi_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[lead_ind]))->Phi(),"full_mva_trees");
        l.FillTree(Form("sublead_phi_%s_Down",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
        l.FillTree(Form("sublead_phi_%s_Up",name.c_str()),((TVector3*)l.sc_xyz->At(l.pho_scind[sublead_ind]))->Phi(),"full_mva_trees");
        */
	}
	// feed the modified signal model to the RooContainer
	l.rooContainer->InputSystematicSet("sig_"+GetSignalLabel(cur_type, l),name,categories,mass_errors,weights);
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::computeExclusiveCategory(LoopAll & l, int & category, std::pair<int,int> diphoton_index, float pt, float diphobdt_output)
{
    //ComputeExclusiveCategory decide la priorita' nella categorizzazione
    if(Looseevent) {//GIUSEPPE controllare_THq
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+((int)includeTTHhad)+((int)includetHqLeptonic)+((int)includeTprimelep)+((int)includeTprimehad)
	    + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
	cout<<"Loose: "<<category<<endl;//MIO
	if(FMDEBUG)
	    cout<<"Loose: "<<category<<endl;
    }else if(Tprimelepevent) {//GIUSEPPE
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep) +((int)includeTTHhad)+((int)includetHqLeptonic)
	    	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);

	cout<<"Tprimelep: MIO"<<category<<endl;//MIO
	//cout<<"in lep::l.Diphoton: MIO"<< l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1)<<endl;//MIO
	if(FMDEBUG)
	cout<<"Tprimelep: "<<category<<endl;
    }else if(Tprimehadevent) {//GIUSEPPE controllare_THq
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+((int)includeTTHhad)+((int)includetHqLeptonic)+((int)includeTprimelep)
	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
	cout<<"Tprimehad: "<<category<<endl;//MIO
	if(FMDEBUG)
	cout<<"Tprimehad: "<<category<<endl;
    } else if (tHqLeptonicevent) {
	category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+ ((int)includeTTHhad)+( (int)includeVHhad )*nVHhadEtaCategories +((int)includeVHhadBtag)+ nVHlepCategories+VHmetevent_cat;
// 	$Id$	
	category+=((int)includetHqLeptonic)*ntHqLeptonicCategories;
       	cout<<"tHqLeptonic"<<category<<endl;
    } else  if(TTHlepevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + 
	    	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
	cout<<"TTHlep: MIO"<<category<<endl;
	if(FMDEBUG)
	    cout<<"TTHlep: "<<category<<endl;
    }else if(TTHhadevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+
	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
	cout<<"TTHhad: MIO"<<category<<endl;//MIO
	if(FMDEBUG)
	cout<<"TTHhad: "<<category<<endl;
    } else if(VHmuevent || VHlep1event) {  //ANIELLO
	category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ( (int)includeVHhad )*nVHhadEtaCategories+((int)includeVHhadBtag)+((int)includeTTHhad)+((int)includeTTHlep);
	if(nMuonCategories>1) category+=VHmuevent_cat;
	cout<<"VHmu: "<<category<<endl;
	if(FMDEBUG)
	cout<<"VHmu: "<<category<<endl;
    } else  if(VHelevent || VHlep2event) {  //ANIELLO
	    category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ( (int)includeVHhad )*nVHhadEtaCategories + ((int)includeVHhadBtag)+((int)includeTTHhad)+((int)includeTTHlep)+ nMuonCategories;
        if(nElectronCategories>1) category+=VHelevent_cat;
	cout<<"VHele: "<<category<<endl;
		if(FMDEBUG)
	cout<<"VHele: "<<category<<endl;
    } else if(VBFevent) {
	    category=nInclusiveCategories_;
	    if( mvaVbfSelection ) {
	        if (!multiclassVbfSelection) {
		    category += categoryFromBoundaries(mvaVbfCatBoundaries, myVBF_MVA);
		} else if ( vbfVsDiphoVbfSelection ) {
		    category += categoryFromBoundaries2D(multiclassVbfCatBoundaries0, multiclassVbfCatBoundaries1, multiclassVbfCatBoundaries2, myVBF_MVA, diphobdt_output, 1.);
	        } else {
		    category += categoryFromBoundaries2D(multiclassVbfCatBoundaries0, multiclassVbfCatBoundaries1, multiclassVbfCatBoundaries2, myVBF_MVA0, myVBF_MVA1, myVBF_MVA2);
		}
	    }
 	    else {
	        category += l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVBFEtaCategories,1,1)
		    + nVBFEtaCategories*l.DijetSubCategory(myVBF_Mjj,myVBFLeadJPt,myVBFSubJPt,nVBFDijetJetCategories);
		if(FMDEBUG)
		cout<<"VBF: "<<category<<endl;
	    }
    }else if(VHmetevent) {
	    category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+ ((int)includeTTHhad)+( (int)includeVHhad )*nVHhadEtaCategories +((int)includeVHhadBtag)+ nVHlepCategories;
        if(nVHmetCategories>1) category+=VHmetevent_cat;
	if(FMDEBUG)
	cout<<"VHmet: "<<category<<endl;
    } else if(VHhadBtagevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+ ((int)includeTTHhad)+
	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
		if(FMDEBUG)
	cout<<"VHhad-btag: "<<category<<endl;
    } else if(VHhadevent) {
        category=nInclusiveCategories_ + ( (int)includeVBF )*nVBFCategories + ((int)includeTTHlep)+ ((int)includeTTHhad)+((int)includeVHhadBtag)+
	        + l.DiphotonCategory(diphoton_index.first,diphoton_index.second,pt,nVHhadEtaCategories,1,1);
		if(FMDEBUG)
	cout<<"VHhad: "<<category<<endl;
    }
    cout<<"Mannaggia: category di computeExclusiveCat"<<category<<endl;

}

int StatAnalysis::categoryFromBoundaries(std::vector<float> & v, float val)
{
    if( val == v[0] ) { return 0; }
    std::vector<float>::iterator bound =  lower_bound( v.begin(), v.end(), val, std::greater<float>  ());
    int cat = ( val >= *bound ? bound - v.begin() - 1 : bound - v.begin() );
    if( cat >= v.size() - 1 ) { cat = -1; }
    return cat;
}

int StatAnalysis::categoryFromBoundaries2D(std::vector<float> & v1, std::vector<float> & v2, std::vector<float> & v3, float val1, float val2, float val3 )
{
    int cat1temp =  categoryFromBoundaries(v1,val1);
    int cat2temp =  categoryFromBoundaries(v2,val2);
    int cat3temp =  categoryFromBoundaries(v3,val3);
    std::vector<int> vcat;
    vcat.push_back(cat1temp);
    vcat.push_back(cat2temp);
    vcat.push_back(cat3temp);
    int cat = *max_element(vcat.begin(), vcat.end());
    return cat;
}

// ----------------------------------------------------------------------------------------------------

void StatAnalysis::fillControlPlots(const TLorentzVector & lead_p4, const  TLorentzVector & sublead_p4, const TLorentzVector & Higgs,
                    float lead_r9, float sublead_r9, int diphoton_id,
                    int category, bool isCorrectVertex, float evweight, TVector3* vtx, LoopAll & l,
                    int muVtx, int mu_ind, int elVtx, int el_ind, float diphobdt_output)
{
    int cur_type = l.itype[l.current];
    float mass = Higgs.M();
    if(category!=-10){  // really this is nomva cut but -1 means all together here
        if( category>=0 ) {
            fillControlPlots( lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, diphoton_id, -1, isCorrectVertex, evweight,
			      vtx, l, muVtx, mu_ind, elVtx, el_ind, diphobdt_output );
        }
        l.FillHist("all_mass",category+1, Higgs.M(), evweight);
        if( mass>=massMin && mass<=massMax  ) {
            l.FillHist("process_id",category+1, l.process_id, evweight);
            l.FillHist("mass",category+1, Higgs.M(), evweight);
            l.FillHist("eta",category+1, Higgs.Eta(), evweight);
            l.FillHist("pt",category+1, Higgs.Pt(), evweight);
            if( isCorrectVertex ) { l.FillHist("pt_rv",category+1, Higgs.Pt(), evweight); }
            l.FillHist("nvtx",category+1, l.vtx_std_n, evweight);
            if( isCorrectVertex ) { l.FillHist("nvtx_rv",category+1, l.vtx_std_n, evweight); }

            vtxAna_.setPairID(diphoton_id);
            float vtxProb = vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n);
            l.FillHist2D("probmva_pt",category+1, Higgs.Pt(), l.vtx_std_evt_mva->at(diphoton_id), evweight);
            l.FillHist2D("probmva_nvtx",category+1, l.vtx_std_n, l.vtx_std_evt_mva->at(diphoton_id), evweight);
            if( isCorrectVertex ) {
                l.FillHist2D("probmva_rv_nvtx",category+1, l.vtx_std_n, l.vtx_std_evt_mva->at(diphoton_id), evweight);
            }
            l.FillHist2D("vtxprob_pt",category+1, Higgs.Pt(), vtxProb, evweight);
            l.FillHist2D("vtxprob_nvtx",category+1, l.vtx_std_n, vtxProb, evweight);
            std::vector<int> & vtxlist = l.vtx_std_ranked_list->at(diphoton_id);
            size_t maxv = std::min(vtxlist.size(),(size_t)5);
            for(size_t ivtx=0; ivtx<maxv; ++ivtx) {
                int vtxid = vtxlist.at(ivtx);
                l.FillHist(Form("vtx_mva_%d",ivtx),category+1,vtxAna_.mva(ivtx),evweight);
                if( ivtx > 0 ) {
                    l.FillHist(Form("vtx_dz_%d",ivtx),category+1,
                       vtxAna_.vertexz(ivtx)-vtxAna_.vertexz(l.dipho_vtxind[diphoton_id]),evweight);
                }
            }
            l.FillHist("vtx_nconv",vtxAna_.nconv(0));

            l.FillHist("pho_pt",category+1,lead_p4.Pt(), evweight);
            l.FillHist("pho1_pt",category+1,lead_p4.Pt(), evweight);
            l.FillHist("pho_eta",category+1,lead_p4.Eta(), evweight);
            l.FillHist("pho1_eta",category+1,lead_p4.Eta(), evweight);
            l.FillHist("pho_r9",category+1, lead_r9, evweight);
            l.FillHist("pho1_r9",category+1, lead_r9, evweight);

            l.FillHist("pho_pt",category+1,sublead_p4.Pt(), evweight);
            l.FillHist("pho2_pt",category+1,sublead_p4.Pt(), evweight);
            l.FillHist("pho_eta",category+1,sublead_p4.Eta(), evweight);
            l.FillHist("pho2_eta",category+1,sublead_p4.Eta(), evweight);
            l.FillHist("pho_r9",category+1, sublead_r9, evweight);
            l.FillHist("pho2_r9",category+1, sublead_r9, evweight);

            l.FillHist("pho_n",category+1,l.pho_n, evweight);

            l.FillHist("pho_rawe",category+1,l.sc_raw[l.pho_scind[l.dipho_leadind[diphoton_id]]], evweight);
            l.FillHist("pho_rawe",category+1,l.sc_raw[l.pho_scind[l.dipho_subleadind[diphoton_id]]], evweight);

            TLorentzVector myMet = l.METCorrection2012B(lead_p4, sublead_p4);
            float corrMet    = myMet.Pt();
            float corrMetPhi = myMet.Phi();

            l.FillHist("uncorrmet",     category+1, l.met_pfmet, evweight);
            l.FillHist("uncorrmetPhi",  category+1, l.met_phi_pfmet, evweight);
            l.FillHist("corrmet",       category+1, corrMet,    evweight);
            l.FillHist("corrmetPhi",    category+1, corrMetPhi, evweight);

            if( mvaVbfSelection ) {
                if (!multiclassVbfSelection) {
                    l.FillHist("vbf_mva",category+1,myVBF_MVA,evweight);
                } else {
                    l.FillHist("vbf_mva0",category+1,myVBF_MVA0,evweight);
                    l.FillHist("vbf_mva1",category+1,myVBF_MVA1,evweight);
                    l.FillHist("vbf_mva2",category+1,myVBF_MVA2,evweight);
                }

                if (VBFevent){
                    float myweight =  1;
                    float sampleweight = l.sampleContainer[l.current_sample_index].weight;
                    if(evweight*sampleweight!=0) myweight=evweight/sampleweight;
                    l.FillCutPlots(category+1,1,"_sequential",evweight,myweight);
		    if( sublead_r9 > 0.9 ) { l.FillCutPlots(category+1+nCategories_,1,"_sequential",evweight,myweight); }
                }
            }
	    l.FillHist("rho",category+1,l.rho_algo1,evweight);


            if(category!=-1){
                bool isEBEB  = fabs(lead_p4.Eta() < 1.4442 ) && fabs(sublead_p4.Eta()<1.4442);
                std::string label("final");
                if (VHelevent){
                    ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
                    l.FillHist("ElectronTag_sameVtx",   (int)isEBEB, (float)(elVtx==l.dipho_vtxind[diphoton_id]), evweight);
                    if(cur_type!=0){
                        l.FillHist(Form("ElectronTag_dZtogen_%s",label.c_str()),    (int)isEBEB, (float)((*vtx - *((TVector3*)l.gv_pos->At(0))).Z()), evweight);
                    }

                }

                if (VHmuevent){
                    ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
                    l.FillHist("MuonTag_sameVtx",   (int)isEBEB, (float)(muVtx==l.dipho_vtxind[diphoton_id]), evweight);
                    if(cur_type!=0){
                        l.FillHist(Form("MuonTag_dZtogen_%s",label.c_str()),   (int)isEBEB, (float)((*vtx - *((TVector3*)l.gv_pos->At(0))).Z()), evweight);
                    }
                }

                if (VHmetevent){
                    ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
                    l.FillHist("METTag_sameVtx",   (int)isEBEB, (float)(0==l.dipho_vtxind[diphoton_id]), evweight);
                }

                label.clear();
                label+="nometcut";
                ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);

                label.clear();
                label+="nomvacut";
                if (VHmuevent){
                    ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
                }
                if (VHelevent){
                    ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
                }
                if (VHmetevent){
                    ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
                }

            }
        }
    } else if( mass>=massMin && mass<=massMax  )  { // is -10 = no mva cut
        std::string label("nomvacut");
        if (VHmuevent){
            ControlPlotsMuonTag2012B(l, lead_p4, sublead_p4, mu_ind, diphobdt_output, evweight, label);
        }
        if (VHelevent){
            ControlPlotsElectronTag2012B(l, lead_p4, sublead_p4, el_ind, diphobdt_output, evweight, label);
        }
        if (VHmetevent){
            ControlPlotsMetTag2012B(l, lead_p4, sublead_p4, diphobdt_output, evweight, label);
        }
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::fillSignalEfficiencyPlots(float weight, LoopAll & l)
{
    //Fill histograms to use as denominator (kinematic pre-selection only) and numerator (selection applied)
    //for single photon ID efficiency calculation.
    int diphoton_id_kinpresel = l.DiphotonMITPreSelection(leadEtCut,subleadEtCut,-1.,applyPtoverM, &smeared_pho_energy[0], -1, false, true );
    if (diphoton_id_kinpresel>-1) {

	TLorentzVector lead_p4, sublead_p4, Higgs;
	float lead_r9, sublead_r9;
	TVector3 * vtx;
	fillDiphoton(lead_p4, sublead_p4, Higgs, lead_r9, sublead_r9, vtx, &smeared_pho_energy[0], l, diphoton_id_kinpresel);

	int ivtx = l.dipho_vtxind[diphoton_id_kinpresel];
	int lead = l.dipho_leadind[diphoton_id_kinpresel];
	int sublead = l.dipho_subleadind[diphoton_id_kinpresel];
	int leadpho_category = l.PhotonCategory(lead, 2, 2);
	int subleadpho_category = l.PhotonCategory(sublead, 2, 2);
	float leadEta = ((TVector3 *)l.sc_xyz->At(l.pho_scind[lead]))->Eta();
	float subleadEta = ((TVector3 *)l.sc_xyz->At(l.pho_scind[sublead]))->Eta();

	float evweight = weight * smeared_pho_weight[lead] * smeared_pho_weight[sublead] * genLevWeight;

	//Fill eta and pt distributions after pre-selection only (efficiency denominator)
	l.FillHist("pho1_pt_presel",0,lead_p4.Pt(), evweight);
	l.FillHist("pho2_pt_presel",0,sublead_p4.Pt(), evweight);
	l.FillHist("pho1_eta_presel",0,leadEta, evweight);
	l.FillHist("pho2_eta_presel",0,subleadEta, evweight);

	l.FillHist("pho1_pt_presel",leadpho_category+1,lead_p4.Pt(), evweight);
	l.FillHist("pho2_pt_presel",subleadpho_category+1,sublead_p4.Pt(), evweight);
	l.FillHist("pho1_eta_presel",leadpho_category+1,leadEta, evweight);
	l.FillHist("pho2_eta_presel",subleadpho_category+1,subleadEta, evweight);

	//Apply single photon CiC selection and fill eta and pt distributions (efficiency numerator)
	std::vector<std::vector<bool> > ph_passcut;
	if( l.PhotonCiCSelectionLevel(lead, ivtx, ph_passcut, 4, 0, &smeared_pho_energy[0]) >=  (LoopAll::phoCiCIDLevel) l.phoSUPERTIGHT) {
	    l.FillHist("pho1_pt_sel",0,lead_p4.Pt(), evweight);
	    l.FillHist("pho1_eta_sel",0,leadEta, evweight);
	    l.FillHist("pho1_pt_sel",leadpho_category+1,lead_p4.Pt(), evweight);
	    l.FillHist("pho1_eta_sel",leadpho_category+1,leadEta, evweight);
	}
	if( l.PhotonCiCSelectionLevel(sublead, ivtx, ph_passcut, 4, 1, &smeared_pho_energy[0]) >=  (LoopAll::phoCiCIDLevel) l.phoSUPERTIGHT ) {
	    l.FillHist("pho2_pt_sel",0,sublead_p4.Pt(), evweight);
	    l.FillHist("pho2_eta_sel",0,subleadEta, evweight);
	    l.FillHist("pho2_pt_sel",subleadpho_category+1,sublead_p4.Pt(), evweight);
	    l.FillHist("pho2_eta_sel",subleadpho_category+1,subleadEta, evweight);
	}
    }
}

// ----------------------------------------------------------------------------------------------------
void StatAnalysis::GetBranches(TTree *t, std::set<TBranch *>& s )
{
    vtxAna_.setBranchAdresses(t,"vtx_std_");
    vtxAna_.getBranches(t,"vtx_std_",s);
}

// ----------------------------------------------------------------------------------------------------
bool StatAnalysis::SelectEvents(LoopAll& l, int jentry)
{
    return true;
}
// ----------------------------------------------------------------------------------------------------
double StatAnalysis::GetDifferentialKfactor(double gPT, int Mass)
{
    return 1.0;
}

void StatAnalysis::FillSignalLabelMap(LoopAll & l)
{
    std::map<int,std::pair<TString,double > > & signalMap = l.signalNormalizer->SignalType();

    for( std::map<int,std::pair<TString,double > >::iterator it=signalMap.begin();
	 it!=signalMap.end(); ++it ) {
	signalLabels[it->first] = it->second.first+Form("_mass_m%1.0f", it->second.second);
    }

    /////////// // Basically A Map of the ID (type) to the signal's name which can be filled Now:
    /////////// signalLabels[-57]="ggh_mass_m123";
    /////////// signalLabels[-58]="vbf_mass_m123";
    /////////// signalLabels[-60]="wzh_mass_m123";
    /////////// signalLabels[-59]="tth_mass_m123";
    /////////// signalLabels[-53]="ggh_mass_m121";
    /////////// signalLabels[-54]="vbf_mass_m121";
    /////////// signalLabels[-56]="wzh_mass_m121";
    /////////// signalLabels[-55]="tth_mass_m121";
    /////////// signalLabels[-65]="ggh_mass_m160";
    /////////// signalLabels[-66]="vbf_mass_m160";
    /////////// signalLabels[-68]="wzh_mass_m160";
    /////////// signalLabels[-67]="tth_mass_m160";
    /////////// signalLabels[-61]="ggh_mass_m155";
    /////////// signalLabels[-62]="vbf_mass_m155";
    /////////// signalLabels[-64]="wzh_mass_m155";
    /////////// signalLabels[-63]="tth_mass_m155";
    /////////// signalLabels[-49]="ggh_mass_m150";
    /////////// signalLabels[-50]="vbf_mass_m150";
    /////////// signalLabels[-52]="wzh_mass_m150";
    /////////// signalLabels[-51]="tth_mass_m150";
    /////////// signalLabels[-45]="ggh_mass_m145";
    /////////// signalLabels[-46]="vbf_mass_m145";
    /////////// signalLabels[-48]="wzh_mass_m145";
    /////////// signalLabels[-47]="tth_mass_m145";
    /////////// signalLabels[-33]="ggh_mass_m140";
    /////////// signalLabels[-34]="vbf_mass_m140";
    /////////// signalLabels[-36]="wzh_mass_m140";
    /////////// signalLabels[-35]="tth_mass_m140";
    /////////// signalLabels[-41]="ggh_mass_m135";
    /////////// signalLabels[-42]="vbf_mass_m135";
    /////////// signalLabels[-44]="wzh_mass_m135";
    /////////// signalLabels[-43]="tth_mass_m135";
    /////////// signalLabels[-29]="ggh_mass_m130";
    /////////// signalLabels[-30]="vbf_mass_m130";
    /////////// signalLabels[-32]="wzh_mass_m130";
    /////////// signalLabels[-31]="tth_mass_m130";
    /////////// signalLabels[-37]="ggh_mass_m125";
    /////////// signalLabels[-38]="vbf_mass_m125";
    /////////// signalLabels[-40]="wzh_mass_m125";
    /////////// signalLabels[-39]="tth_mass_m125";
    /////////// signalLabels[-25]="ggh_mass_m120";
    /////////// signalLabels[-26]="vbf_mass_m120";
    /////////// signalLabels[-28]="wzh_mass_m120";
    /////////// signalLabels[-27]="tth_mass_m120";
    /////////// signalLabels[-21]="ggh_mass_m115";
    /////////// signalLabels[-22]="vbf_mass_m115";
    /////////// signalLabels[-24]="wzh_mass_m115";
    /////////// signalLabels[-23]="tth_mass_m115";
    /////////// signalLabels[-17]="ggh_mass_m110";
    /////////// signalLabels[-18]="vbf_mass_m110";
    /////////// signalLabels[-19]="wzh_mass_m110";
    /////////// signalLabels[-20]="tth_mass_m110";
    /////////// signalLabels[-13]="ggh_mass_m105";
    /////////// signalLabels[-14]="vbf_mass_m105";
    /////////// signalLabels[-16]="wzh_mass_m105";
    /////////// signalLabels[-15]="tth_mass_m105";
    /////////// signalLabels[-69]="ggh_mass_m100";
    /////////// signalLabels[-70]="vbf_mass_m100";
    /////////// signalLabels[-72]="wzh_mass_m100";
    /////////// signalLabels[-71]="tth_mass_m100";
}

std::string StatAnalysis::GetSignalLabel(int id, LoopAll &l){

    // For the lazy man, can return a memeber of the map rather than doing it yourself
    std::map<int,std::string>::iterator it = signalLabels.find(id);

    if (it!=signalLabels.end()){
        if(!splitwzh){
            return it->second;
        } else {
            std::string returnstr = it->second;
            if (l.process_id==26){   // wh event
                returnstr.replace(0, 3, "wh");
            } else if (l.process_id==24){   // zh event
                returnstr.replace(0, 3, "zh");
            }
            return returnstr;
        }

    } else {

        std::cerr << "No Signal Type defined in map with id - " << id << std::endl;
        return "NULL";
    }

}

void StatAnalysis::rescaleClusterVariables(LoopAll &l){

    // Data-driven MC scalings
    for (int ipho=0;ipho<l.pho_n;ipho++){

	if (dataIs2011) {

	    if( scaleR9Only ) {
		double R9_rescale = (l.pho_isEB[ipho]) ? 1.0048 : 1.00492 ;
		l.pho_r9[ipho]*=R9_rescale;
	    } else {
		l.pho_r9[ipho]*=1.0035;
		if (l.pho_isEB[ipho]){ l.pho_sieie[ipho] = (0.87*l.pho_sieie[ipho]) + 0.0011 ;}
		else {l.pho_sieie[ipho]*=0.99;}
		l.sc_seta[l.pho_scind[ipho]]*=0.99;
		l.sc_sphi[l.pho_scind[ipho]]*=0.99;
		energyCorrectedError[ipho] *=(l.pho_isEB[ipho]) ? 1.07 : 1.045 ;
	    }

	} else {
	    //2012 rescaling from here https://hypernews.cern.ch/HyperNews/CMS/get/higgs2g/752/1/1/2/1/3.html

	    if (l.pho_isEB[ipho]) {
		l.pho_r9[ipho] = 1.0045*l.pho_r9[ipho] + 0.0010;
	    } else {
		l.pho_r9[ipho] = 1.0086*l.pho_r9[ipho] - 0.0007;
	    }
	    if( !scaleR9Only ) {
		if (l.pho_isEB[ipho]) {
		    l.pho_s4ratio[ipho] = 1.01894*l.pho_s4ratio[ipho] - 0.01034;
		    l.pho_sieie[ipho] = 0.891832*l.pho_sieie[ipho] + 0.0009133;
		    l.pho_etawidth[ipho] =  1.04302*l.pho_etawidth[ipho] - 0.000618;
		    l.sc_sphi[l.pho_scind[ipho]] =  1.00002*l.sc_sphi[l.pho_scind[ipho]] - 0.000371;
		} else {
		    l.pho_s4ratio[ipho] = 1.04969*l.pho_s4ratio[ipho] - 0.03642;
		    l.pho_sieie[ipho] = 0.99470*l.pho_sieie[ipho] + 0.00003;
		    l.pho_etawidth[ipho] =  0.903254*l.pho_etawidth[ipho] + 0.001346;
		    l.sc_sphi[l.pho_scind[ipho]] =  0.99992*l.sc_sphi[l.pho_scind[ipho]] - 0.00000048;
		    //Agreement not to rescale ES shape (https://hypernews.cern.ch/HyperNews/CMS/get/higgs2g/789/1/1/1/1/1/1/2/1/1.html)
		    //if (l.pho_ESEffSigmaRR[ipho]>0) l.pho_ESEffSigmaRR[ipho] = 1.00023*l.pho_ESEffSigmaRR[ipho] + 0.0913;
		}
	    }
	}
	// Scale DYJets sample for now
	/*
	  if (l.itype[l.current]==6){
	  if (l.pho_isEB[ipho]) {
	  energyCorrectedError[ipho] = 1.02693*energyCorrectedError[ipho]-0.0042793;
	  } else {
	  energyCorrectedError[ipho] = 1.01372*energyCorrectedError[ipho]+0.000156943;
	  }
	  }
	*/
    }
}

void StatAnalysis::ResetAnalysis(){
    // Reset Random Variable on the EnergyResolution Smearer
    if( doEresolSmear ) {
	eResolSmearer->resetRandom();
    }
}

void dumpJet(std::ostream & eventListText, int lab, LoopAll & l, int ijet)
{
    eventListText << std::setprecision(4) << std::scientific
		  << "\tjec"      << lab << ":" << l.jet_algoPF1_erescale[ijet]
		  << "\tbetaStar" << lab << ":" << l.jet_algoPF1_betaStarClassic[ijet]
		  << "\tRMS"      << lab << ":" << l.jet_algoPF1_dR2Mean[ijet]
	;
}

void dumpPhoton(std::ostream & eventListText, int lab,
		LoopAll & l, int ipho, int ivtx, TLorentzVector & phop4, float * pho_energy_array)
{
    float val_tkisobad = -99;
    for(int iv=0; iv < l.vtx_std_n; iv++) {
	if((*l.pho_pfiso_mycharged04)[ipho][iv] > val_tkisobad) {
	    val_tkisobad = (*l.pho_pfiso_mycharged04)[ipho][iv];
	}
    }
    TLorentzVector phop4_badvtx = l.get_pho_p4( ipho, l.pho_tkiso_badvtx_id[ipho], pho_energy_array  );

    float val_tkiso        = (*l.pho_pfiso_mycharged03)[ipho][ivtx];
    float val_ecaliso      = l.pho_pfiso_myphoton03[ipho];
    float val_ecalisobad   = l.pho_pfiso_myphoton04[ipho];
    float val_sieie        = l.pho_sieie[ipho];
    float val_hoe          = l.pho_hoe[ipho];
    float val_r9           = l.pho_r9[ipho];
    float val_conv         = l.pho_isconv[ipho];

    float rhofacbad=0.23, rhofac=0.09;

    float val_isosumoet    = (val_tkiso    + val_ecaliso    - l.rho_algo1 * rhofac )   * 50. / phop4.Et();
    float val_isosumoetbad = (val_tkisobad + val_ecalisobad - l.rho_algo1 * rhofacbad) * 50. / phop4_badvtx.Et();

    // tracker isolation cone energy divided by Et
    float val_trkisooet    = (val_tkiso) * 50. / phop4.Pt();

    eventListText << std::setprecision(4) << std::scientific
		  << "\tchIso03"  << lab << ":" << val_tkiso
		  << "\tphoIso03" << lab << ":" << val_ecaliso
		  << "\tchIso04"  << lab << ":" << val_tkisobad
		  << "\tphoIso04" << lab << ":" << val_ecalisobad
		  << "\tsIeIe"    << lab << ":" << val_sieie
		  << "\thoe"      << lab << ":" << val_hoe
		  << "\tecalIso"  << lab << ":" << l.pho_ecalsumetconedr03[ipho]
		  << "\thcalIso"  << lab << ":" << l.pho_hcalsumetconedr03[ipho]
		  << "\ttrkIso"   << lab << ":" << l.pho_trksumpthollowconedr03[ipho]
		  << "\tchIso02"  << lab << ":" << (*l.pho_pfiso_mycharged02)[ipho][ivtx]
		  << "\teleVeto"  << lab << ":" << !val_conv
	;
}


void StatAnalysis::fillOpTree(LoopAll& l, const TLorentzVector & lead_p4, const TLorentzVector & sublead_p4, float *smeared_pho_energy, Float_t vtxProb,
        std::pair<int, int> diphoton_index, Int_t diphoton_id, Float_t phoid_mvaout_lead, Float_t phoid_mvaout_sublead,
        Float_t weight, Float_t evweight, Float_t mass, Float_t sigmaMrv, Float_t sigmaMwv,
        const TLorentzVector & Higgs, Float_t diphobdt_output, Int_t category, bool VBFevent, Float_t myVBF_Mjj, Float_t myVBFLeadJPt, 
        Float_t myVBFSubJPt, Int_t nVBFDijetJetCategories, bool isSyst, std::string name1) {

    
    // event variables
    //TREE DI CONTROLLO
    
    int njets=0;//my new variable
    int nbjets_medium=0;//new variable
    int nbjets_loose=0;//new variable
    double Ht=0;//new variable




    /*    l.FillTree("nbjets_loose",nbjets_loose);
	  l.FillTree("nbjets_medium", nbjets_medium);*///GIUSEPPE SelectJets implemented in GeneralFunctions.cc!!!CONTROLLARE!!!!
    //l.FillTree("njets", (l.SelectJets).size());//GIUSEPPE SelectJets implemented in GeneralFunctions.cc!!!CONTROLLARE!!!!
    //l.FillTree("Ht",lead_p4.Pt()+l.met_pfmet+);//GIUSEPPE VEDERE
    l.FillTree("itype", (int)l.itype[l.current]);
    l.FillTree("run",(int)l.run);
    l.FillTree("lumis",(int)l.lumis);
    l.FillTree("event",(unsigned int)l.event);
    l.FillTree("weight",(float)weight);
	l.FillTree("evweight",(float)evweight);
	float pu_weight = weight/l.sampleContainer[l.current_sample_index].weight; // contains also the smearings, not only pu
	l.FillTree("pu_weight",(float)pu_weight);
	l.FillTree("pu_n",(float)l.pu_n);
	l.FillTree("nvtx",(float)l.vtx_std_n);
	l.FillTree("rho", (float)l.rho_algo1);
	l.FillTree("category", (int)category);//capire la categorizzazione!!!!

// photon variables
	l.FillTree("ph1_drGsf",l.pho_drtotk_25_99[diphoton_index.first]);
	l.FillTree("ph2_drGsf",l.pho_drtotk_25_99[diphoton_index.second]);
	l.FillTree("ph1_e",(float)lead_p4.E());
	l.FillTree("ph2_e",(float)sublead_p4.E());
	l.FillTree("ph1_pt",(float)lead_p4.Pt());
	l.FillTree("ph2_pt",(float)sublead_p4.Pt());
	l.FillTree("ph1_phi",(float)lead_p4.Phi());
	l.FillTree("ph2_phi",(float)sublead_p4.Phi());
	l.FillTree("ph1_eta",(float)lead_p4.Eta());
	l.FillTree("ph2_eta",(float)sublead_p4.Eta());
	l.FillTree("ph1_r9",(float)l.pho_r9[diphoton_index.first]);
	l.FillTree("ph2_r9",(float)l.pho_r9[diphoton_index.second]);
//	l.FillTree("ph1_isPrompt", (int)l.GenParticleInfo(diphoton_index.first, l.dipho_vtxind[diphoton_id], 0.1));
//	l.FillTree("ph2_isPrompt", (int)l.GenParticleInfo(diphoton_index.second, l.dipho_vtxind[diphoton_id], 0.1));
	l.FillTree("ph1_isPrompt", (int)l.pho_genmatched[diphoton_index.first]); // Alternative definition for gen photon matching: Nicolas' definition need re-reduction
	l.FillTree("ph2_isPrompt", (int)l.pho_genmatched[diphoton_index.second]);
	l.FillTree("ph1_SCEta", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.first]))->Eta());
	l.FillTree("ph2_SCEta", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.second]))->Eta());
	l.FillTree("ph1_SCPhi", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.first]))->Phi());
	l.FillTree("ph2_SCPhi", (float)((TVector3 *)l.sc_xyz->At(l.pho_scind[diphoton_index.second]))->Phi());
	l.FillTree("ph1_hoe", (float)l.pho_hoe[diphoton_index.first]);
	l.FillTree("ph2_hoe", (float)l.pho_hoe[diphoton_index.second]);
	l.FillTree("ph1_sieie", (float)l.pho_sieie[diphoton_index.first]);
	l.FillTree("ph2_sieie", (float)l.pho_sieie[diphoton_index.second]);
	l.FillTree("ph1_pfchargedisogood03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.first][l.dipho_vtxind[diphoton_id]]);
	l.FillTree("ph2_pfchargedisogood03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.second][l.dipho_vtxind[diphoton_id]]);
	double pho1_pfchargedisobad04 = 0.;
	double pho2_pfchargedisobad04 = 0.;
	double pho1_pfchargedisobad03 = 0.;
	double pho2_pfchargedisobad03 = 0.;
	int pho1_ivtxpfch04bad=-1;

	for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++){
        if ((*l.pho_pfiso_mycharged04)[diphoton_index.first][ivtx]>pho1_pfchargedisobad04){
            pho1_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.first][ivtx]>pho1_pfchargedisobad04;
	        pho1_ivtxpfch04bad = ivtx;
    }
	}
	pho1_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.first][pho1_ivtxpfch04bad];
	l.FillTree("ph1_pfchargedisobad04", (float)pho1_pfchargedisobad04);
   int pho2_ivtxpfch04bad=-1;
    for(int ivtx=0; ivtx<l.vtx_std_n; ivtx++){
      if ((*l.pho_pfiso_mycharged04)[diphoton_index.second][ivtx]>pho2_pfchargedisobad04){
        pho2_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.second][ivtx]>pho2_pfchargedisobad04;
          pho2_ivtxpfch04bad = ivtx;
      }
    }
    pho2_pfchargedisobad04=(*l.pho_pfiso_mycharged04)[diphoton_index.second][pho2_ivtxpfch04bad];
	l.FillTree("ph2_pfchargedisobad04", (float)pho2_pfchargedisobad04);
	l.FillTree("ph1_etawidth", (float)l.pho_etawidth[diphoton_index.first]);
	l.FillTree("ph2_etawidth", (float)l.pho_etawidth[diphoton_index.second]);
	l.FillTree("ph1_phiwidth", (float)(l.sc_sphi[l.pho_scind[diphoton_index.first]]));
	l.FillTree("ph2_phiwidth", (float)(l.sc_sphi[l.pho_scind[diphoton_index.second]]));
	l.FillTree("ph1_eseffssqrt", (float)sqrt(l.pho_eseffsixix[diphoton_index.first]*l.pho_eseffsixix[diphoton_index.first]+l.pho_eseffsiyiy[diphoton_index.first]*l.pho_eseffsiyiy[diphoton_index.first]));
	l.FillTree("ph2_eseffssqrt", (float)sqrt(l.pho_eseffsixix[diphoton_index.second]*l.pho_eseffsixix[diphoton_index.second]+l.pho_eseffsiyiy[diphoton_index.second]*l.pho_eseffsiyiy[diphoton_index.second]));
	l.FillTree("ph1_pfchargedisobad03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.first][pho1_ivtxpfch04bad]);
	l.FillTree("ph2_pfchargedisobad03", (float)(*l.pho_pfiso_mycharged03)[diphoton_index.second][pho2_ivtxpfch04bad]);
	l.FillTree("ph1_sieip", (float)l.pho_sieip[diphoton_index.first]);
	l.FillTree("ph2_sieip", (float)l.pho_sieip[diphoton_index.second]);
	l.FillTree("ph1_sipip", (float)l.pho_sipip[diphoton_index.first]);
	l.FillTree("ph2_sipip", (float)l.pho_sipip[diphoton_index.second]);
	l.FillTree("ph1_ecaliso", (float)l.pho_pfiso_myphoton03[diphoton_index.first]);
	l.FillTree("ph2_ecaliso", (float)l.pho_pfiso_myphoton03[diphoton_index.second]);
	l.FillTree("ph1_ecalisobad", (float)l.pho_pfiso_myphoton04[diphoton_index.first]);
	l.FillTree("ph2_ecalisobad", (float)l.pho_pfiso_myphoton04[diphoton_index.second]);
    TLorentzVector ph1_badvtx = l.get_pho_p4( diphoton_index.first, l.pho_tkiso_badvtx_id[diphoton_index.first], smeared_pho_energy );
	l.FillTree("ph1_badvtx_Et", (float)ph1_badvtx.Et());
    TLorentzVector ph2_badvtx = l.get_pho_p4( diphoton_index.second, l.pho_tkiso_badvtx_id[diphoton_index.second], smeared_pho_energy );
	l.FillTree("ph2_badvtx_Et", (float)ph2_badvtx.Et());
	l.FillTree("ph1_isconv", (float)l.pho_isconv[diphoton_index.first]);
	l.FillTree("ph2_isconv", (float)l.pho_isconv[diphoton_index.second]);
    vector<vector<bool> > ph_passcut;
    int ph1_ciclevel = l.PhotonCiCPFSelectionLevel(diphoton_index.first, l.dipho_vtxind[diphoton_id], ph_passcut, 4, 0, smeared_pho_energy);
    int ph2_ciclevel = l.PhotonCiCPFSelectionLevel(diphoton_index.second, l.dipho_vtxind[diphoton_id], ph_passcut, 4, 0, smeared_pho_energy);
    l.FillTree("ph1_ciclevel", (int)ph1_ciclevel);
    l.FillTree("ph2_ciclevel", (int)ph2_ciclevel);
    l.FillTree("ph1_sigmaEoE", (float)l.pho_regr_energyerr[diphoton_index.first]/(float)l.pho_regr_energy[diphoton_index.first]);
    l.FillTree("ph2_sigmaEoE", (float)l.pho_regr_energyerr[diphoton_index.second]/(float)l.pho_regr_energy[diphoton_index.second]);
    l.FillTree("ph1_ptoM", (float)lead_p4.Pt()/mass);
    l.FillTree("ph2_ptoM", (float)sublead_p4.Pt()/mass);
    l.FillTree("ph1_isEB", (int)l.pho_isEB[diphoton_index.first]);
    l.FillTree("ph2_isEB", (int)l.pho_isEB[diphoton_index.second]);
    float s4ratio1 = l.pho_e2x2[diphoton_index.first]/l.pho_e5x5[diphoton_index.first];
    float s4ratio2 = l.pho_e2x2[diphoton_index.second]/l.pho_e5x5[diphoton_index.second];
    l.FillTree("ph1_s4ratio", s4ratio1);
    l.FillTree("ph2_s4ratio", s4ratio2);
    l.FillTree("ph1_e3x3", l.pho_e3x3[diphoton_index.first]);
    l.FillTree("ph2_e3x3", l.pho_e3x3[diphoton_index.second]);
    l.FillTree("ph1_e5x5", l.pho_e5x5[diphoton_index.first]);
    l.FillTree("ph2_e5x5", l.pho_e5x5[diphoton_index.second]);

// diphoton variables
    l.FillTree("PhotonsMass",(float)mass);
    TLorentzVector diphoton = lead_p4 + sublead_p4;
    l.FillTree("dipho_E", (float)diphoton.Energy());
    l.FillTree("dipho_pt", (float)diphoton.Pt());
    l.FillTree("dipho_eta", (float)diphoton.Eta());
    l.FillTree("dipho_phi", (float)diphoton.Phi());
    // cosThetaStar computation with explicit Collin-Sopper Frame, from M. Peruzzi
    TLorentzVector b1,b2;
    b1.SetPx(0); b1.SetPy(0); b1.SetPz( 4000); b1.SetE(4000);
    b2.SetPx(0); b2.SetPy(0); b2.SetPz(-4000); b2.SetE(4000);
    TLorentzVector boostedpho1 = lead_p4;
    TLorentzVector boostedpho2 = sublead_p4;
    TLorentzVector boostedb1 = b1;
    TLorentzVector boostedb2 = b2;
    TVector3 boost = (lead_p4+sublead_p4).BoostVector();
    boostedpho1.Boost(-boost);
    boostedpho2.Boost(-boost);
    boostedb1.Boost(-boost);
    boostedb2.Boost(-boost);
    TVector3 direction_cs = (boostedb1.Vect().Unit()-boostedb2.Vect().Unit()).Unit();
    float dipho_cosThetaStar_CS = fabs(TMath::Cos(direction_cs.Angle(boostedpho1.Vect())));
    l.FillTree("dipho_cosThetaStar_CS", (float)dipho_cosThetaStar_CS);
    float dipho_tanhYStar = tanh(
        (float)(fabs(lead_p4.Rapidity() - sublead_p4.Rapidity()))/(float)(2.0)
    );
	l.FillTree("dipho_tanhYStar", (float)dipho_tanhYStar);
	l.FillTree("dipho_Y", (float)diphoton.Rapidity());
    TVector3* vtx = (TVector3*)l.vtx_std_xyz->At(l.dipho_vtxind[diphoton_id]);

// vertices variables
    l.FillTree("vtx_ind", (int)l.dipho_vtxind[diphoton_id]);
    l.FillTree("vtx_x", (float)vtx->X());
    l.FillTree("vtx_y", (float)vtx->Y());
    l.FillTree("vtx_z", (float)vtx->Z());
    vtxAna_.setPairID(diphoton_id);
    l.FillTree("vtx_mva", (float)vtxAna_.mva(0));
    l.FillTree("vtx_mva_2", (float)vtxAna_.mva(1));
    l.FillTree("vtx_mva_3", (float)vtxAna_.mva(2));
    l.FillTree("vtx_ptbal", (float)vtxAna_.ptbal(0));
    l.FillTree("vtx_ptasym", (float)vtxAna_.ptasym(0));
    l.FillTree("vtx_logsumpt2", (float)vtxAna_.logsumpt2(0));
    l.FillTree("vtx_pulltoconv", (float)vtxAna_.pulltoconv(0));
    l.FillTree("vtx_prob", (float)vtxAna_.vertexProbability(l.vtx_std_evt_mva->at(diphoton_id), l.vtx_std_n));

// jet variables

    TLorentzVector *lep=new TLorentzVector();//GIUSEPPE
    lep->SetPtEtaPhiM(0.,0.,0.,0.);//GIUSEPPE
    if(l.isLep_ele){//GIUSEPPE
	lep= (TLorentzVector*) l.el_std_p4->At(l.el_ind);
    }else if(l.isLep_mu){
	lep= (TLorentzVector*)l.mu_glo_p4->At(l.mu_ind);
    }
   
    vector<int> jets;
    jets = l.SelectJets_looser(l,diphoton_id,lead_p4,sublead_p4,lep);//my_looser
    //    int elInd = l.ElectronSelectionMVA2012(20.);//GIUSEPPE
    //    int muonInd = l.MuonSelection2012B(20.);//GIUSEPPE



    l.FillTree("njets_passing_kLooseID",(int)jets.size());
    // l.FillTree("njets",(int)jets.size());//GIUSEPPE CONTROLLARE!!!!
    if(jets.size()>0){
        TLorentzVector* jet1 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[0]);
	    l.FillTree("j1_e",(float)jet1->Energy());
    	l.FillTree("j1_pt",(float)jet1->Pt());
	Ht+=(double)jet1->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j1_phi",(float)jet1->Phi());
    	l.FillTree("j1_eta",(float)jet1->Eta());
	    //l.FillTree("j1_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[0]]);
    	l.FillTree("j1_beta", (float)l.jet_algoPF1_beta[jets[0]]);
	    l.FillTree("j1_betaStar", (float)l.jet_algoPF1_betaStar[jets[0]]);
    	l.FillTree("j1_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[0]]);
	    l.FillTree("j1_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[0]]);
	l.FillTree("j1_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[0]]);
	if(l.jet_algoPF1_csvBtag[jets[0]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[0]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//dr_jet_lep= jet1->DeltaR(*lep);//GIUSEPPE
        //if(dr_jet_lep<0.5) {njets++;}
	njets++;
    if(jets.size()>1){
        TLorentzVector* jet2 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[1]);
	    l.FillTree("j2_e",(float)jet2->Energy());
	    l.FillTree("j2_pt",(float)jet2->Pt());
	    Ht+=(double)jet2->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j2_phi",(float)jet2->Phi());
	    l.FillTree("j2_eta",(float)jet2->Eta());
	    //l.FillTree("j2_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[1]]);
    	l.FillTree("j2_beta", (float)l.jet_algoPF1_beta[jets[1]]);
    	l.FillTree("j2_betaStar", (float)l.jet_algoPF1_betaStar[jets[1]]);
    	l.FillTree("j2_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[1]]);
    	l.FillTree("j2_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[1]]);
	l.FillTree("j2_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[1]]);
	if(l.jet_algoPF1_csvBtag[jets[1]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[1]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet2->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;         
// dijet variables

        TLorentzVector dijet; 
	if(!(includeVHhadBtag && (category == 6) && !includeTTHlep)){//VA CAMBIATO?
        dijet = *jet1 + *jet2;
	}else{
	    std::pair<int, int>  jets_map = l.SelectBtaggedAndHighestPtJets(l,diphoton_id,lead_p4, sublead_p4);
	    TLorentzVector* firstjet=(TLorentzVector*)l.jet_algoPF1_p4->At(jets_map.first);
	    TLorentzVector* secondjet=(TLorentzVector*)l.jet_algoPF1_p4->At(jets_map.second);
	    dijet = *firstjet + *secondjet;
	}
	l.FillTree("JetsMass", (float)dijet.M());
        l.FillTree("dijet_E", (float)dijet.Energy());
        l.FillTree("dijet_Pt", (float)dijet.Pt());
        l.FillTree("dijet_Eta", (float)dijet.Eta());
        l.FillTree("dijet_Phi", (float)dijet.Phi());

	TLorentzVector Vstar = dijet + diphoton;
	
	TLorentzVector H_Vstar(diphoton);
	H_Vstar.Boost(-Vstar.BoostVector());
	
	float cosThetaStar = -H_Vstar.CosTheta();
	float abs_cosThetaStar = fabs(cosThetaStar);

        l.FillTree("cosThetaStar", (float)abs_cosThetaStar);

        // radion variables
	/*        TLorentzVector radion = dijet + diphoton;
	    l.FillTree("RadMass",(float)radion.M());
        l.FillTree("radion_E", (float)radion.Energy());
        l.FillTree("radion_Pt", (float)radion.Pt());
        l.FillTree("radion_Eta", (float)radion.Eta());
        l.FillTree("radion_Phi", (float)radion.Phi());*/
    } // if 2 jets

    } // if 1 jet


    TLorentzVector* jet3 = new TLorentzVector();
    TLorentzVector* jet4 = new TLorentzVector();
    TLorentzVector* jet5 = new TLorentzVector();
    TLorentzVector* jet6 = new TLorentzVector();
    TLorentzVector* jet7 = new TLorentzVector();
    TLorentzVector* jet8 = new TLorentzVector();
    TLorentzVector* jet9 = new TLorentzVector();
    TLorentzVector* jet10 = new TLorentzVector();

    if(jets.size() > 2){
        jet3 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[2]);
    	l.FillTree("j3_e",(float)jet3->Energy());
	    l.FillTree("j3_pt",(float)jet3->Pt());
	    Ht+=(double)jet3->Pt();//GIUSEPPE CONTROLLARE
    	l.FillTree("j3_phi",(float)jet3->Phi());
    	l.FillTree("j3_eta",(float)jet3->Eta());
	    //l.FillTree("j3_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[2]]);
	    l.FillTree("j3_beta", (float)l.jet_algoPF1_beta[jets[2]]);
	    l.FillTree("j3_betaStar", (float)l.jet_algoPF1_betaStar[jets[2]]);
	    l.FillTree("j3_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[2]]);
	    l.FillTree("j3_dR2Mean", (float)l.jet_algoPF1_dR2Mean[jets[2]]);
	    l.FillTree("j3_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[2]]);
	if(l.jet_algoPF1_csvBtag[jets[2]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[2]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//dr_jet_lep= jet3->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
} // if 3 jets

    if(jets.size() > 3){
        jet4 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[3]);
    	l.FillTree("j4_e",(float)jet4->Energy());
	    l.FillTree("j4_pt",(float)jet4->Pt());
	    Ht+=(double)jet4->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j4_phi",(float)jet4->Phi());
	    l.FillTree("j4_eta",(float)jet4->Eta());
    
	    //l.FillTree("j4_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[3]]);
	    l.FillTree("j4_beta", (float)l.jet_algoPF1_beta[jets[3]]);
	    l.FillTree("j4_betaStar", (float)l.jet_algoPF1_betaStar[jets[3]]);
	    l.FillTree("j4_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[3]]);
	    l.FillTree("j4_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[3]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[3]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[3]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//        dr_jet_lep= jet4->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
} // if 4 jets

    if(jets.size() > 4){
        jet5 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[4]);
    	l.FillTree("j5_e",(float)jet5->Energy());
	    l.FillTree("j5_pt",(float)jet5->Pt());
	    Ht+=(double)jet5->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j5_phi",(float)jet5->Phi());
	    l.FillTree("j5_eta",(float)jet5->Eta());
    
	    //l.FillTree("j5_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[5]]);
	    l.FillTree("j5_beta", (float)l.jet_algoPF1_beta[jets[4]]);
	    l.FillTree("j5_betaStar", (float)l.jet_algoPF1_betaStar[jets[4]]);
	    l.FillTree("j5_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[4]]);
	    l.FillTree("j5_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[4]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[4]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[4]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet5->DeltaR(*lep);//GIUSEPPE                                                                                                            
	//        if(dr_jet_lep>0.5) {njets++;}
	njets++;

} // if 5 jets

    if(jets.size() > 5){
        jet6 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[5]);
    	l.FillTree("j6_e",(float)jet6->Energy());
	    Ht+=(double)jet6->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j6_pt",(float)jet6->Pt());
	    l.FillTree("j6_phi",(float)jet6->Phi());
	    l.FillTree("j6_eta",(float)jet6->Eta());
    
	    //l.FillTree("j6_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[5]]);
	    l.FillTree("j6_beta", (float)l.jet_algoPF1_beta[jets[5]]);
	    l.FillTree("j6_betaStar", (float)l.jet_algoPF1_betaStar[jets[5]]);
	    l.FillTree("j6_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[5]]);
	    l.FillTree("j6_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[5]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[5]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[5]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet6->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
} // if 6 jets

    if(jets.size() > 6){
        jet7 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[6]);
    	l.FillTree("j7_e",(float)jet7->Energy());
	    Ht+=(double)jet7->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j7_pt",(float)jet7->Pt());
	    l.FillTree("j7_phi",(float)jet7->Phi());
	    l.FillTree("j7_eta",(float)jet7->Eta());
    
	    //l.FillTree("j7_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[6]]);
	    l.FillTree("j7_beta", (float)l.jet_algoPF1_beta[jets[6]]);
	    l.FillTree("j7_betaStar", (float)l.jet_algoPF1_betaStar[jets[6]]);
	    l.FillTree("j7_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[6]]);
	    l.FillTree("j7_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[6]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[6]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[6]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet7->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;

} // if 7 jets

    if(jets.size() > 7){
        jet8 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[7]);
    	l.FillTree("j8_e",(float)jet8->Energy());
	    l.FillTree("j8_pt",(float)jet8->Pt());
	    Ht+=(double)jet8->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j8_phi",(float)jet8->Phi());
	    l.FillTree("j8_eta",(float)jet8->Eta());
    
	    //l.FillTree("j8_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[8]]);
	    l.FillTree("j8_beta", (float)l.jet_algoPF1_beta[jets[7]]);
	    l.FillTree("j8_betaStar", (float)l.jet_algoPF1_betaStar[jets[7]]);
	    l.FillTree("j8_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[7]]);
	    l.FillTree("j8_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[7]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[7]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[7]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet8->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
    } // if 8 jets

    if(jets.size() > 8){
        jet9 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[8]);
    	l.FillTree("j9_e",(float)jet9->Energy());
	    Ht+=(double)jet9->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j9_pt",(float)jet9->Pt());
	    l.FillTree("j9_phi",(float)jet9->Phi());
	    l.FillTree("j9_eta",(float)jet9->Eta());
    
	    //l.FillTree("j9_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[8]]);
	    l.FillTree("j9_beta", (float)l.jet_algoPF1_beta[jets[8]]);
	    l.FillTree("j9_betaStar", (float)l.jet_algoPF1_betaStar[jets[8]]);
	    l.FillTree("j9_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[8]]);
	    l.FillTree("j9_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[8]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[8]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[8]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet9->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
} // if 9 jets

    if(jets.size() > 9){
        jet10 = (TLorentzVector*)l.jet_algoPF1_p4->At(jets[9]);
    	l.FillTree("j10_e",(float)jet10->Energy());
	    l.FillTree("j10_pt",(float)jet10->Pt());
	    Ht+=(double)jet10->Pt();//GIUSEPPE CONTROLLARE
	    l.FillTree("j10_phi",(float)jet10->Phi());
	    l.FillTree("j10_eta",(float)jet10->Eta());
    
	    //l.FillTree("j10_cutbased_wp_level", (float)l.jet_algoPF1_cutbased_wp_level[jets[10]]);
	    l.FillTree("j10_beta", (float)l.jet_algoPF1_beta[jets[9]]);
	    l.FillTree("j10_betaStar", (float)l.jet_algoPF1_betaStar[jets[9]]);
	    l.FillTree("j10_betaStarClassic", (float)l.jet_algoPF1_betaStarClassic[jets[9]]);
	    l.FillTree("j10_algoPF1_csvBtag", (float)l.jet_algoPF1_csvBtag[jets[9]]);   	  
	if(l.jet_algoPF1_csvBtag[jets[9]]>0.244)nbjets_loose++;//GIUSEPPE CONTROLLARE
        if(l.jet_algoPF1_csvBtag[jets[9]]>0.679)nbjets_medium++;//GIUSEPPE CONTROLLARE
	//	dr_jet_lep= jet10->DeltaR(*lep);//GIUSEPPE                                                                                                            
        //if(dr_jet_lep>0.5) {njets++;}
	njets++;
} // if 10 jets
    /*    if(l.isLep_ele||l.isLep_mu){//GIUSEPPE
	cout<<njets<<"STAT"<<endl;//Verifico
	cout<<jets.size()<<"SIZE"<<endl;
	//	int ciao;//BLOCCO
	//      cin>>ciao;//BLOCCO
	}*/

    l.FillTree("njets",njets);
    l.FillTree("nbjets_loose",nbjets_loose);
    l.FillTree("nbjets_medium",nbjets_medium);
    //in Ht also MET phot leptons

    TLorentzVector mylead=lead_p4;
    TLorentzVector mySublead=lead_p4;
    l.FillTree("nelectrons",l.GetNelectronsPassingSelectionMVA2012(10.,mylead,mySublead,deltaRPholep_cut));
    l.FillTree("nmuons",l.GetNMuonsPassingSelection2012B(10.,mylead, mySublead,deltaRPholep_cut));


    l.FillTree("met_pfmet",l.met_pfmet);
    l.FillTree("met_phi_pfmet",l.met_phi_pfmet);

    //    if(l.el_ind >-1 && (category ==6 ||category == 7||(category==11 && includetHqLeptonic))) {
    
    /*APPENA COMMENTATO
    if(l.el_ind >-1 && (category == 6 || (category==11 && includetHqLeptonic))) {//VA SICURAMENTE CAMBIATO
	//	cout<<l.el_ind<<" ";
        TLorentzVector* electron = (TLorentzVector*)l.el_std_p4->At(l.el_ind);
	l.FillTree("ptEle",(float)electron->Pt());
	l.FillTree("etaEle",(float)electron->Eta());
	l.FillTree("phiEle",(float)electron->Phi());
	if((category==11 && includetHqLeptonic))	l.FillTree("chargeEle",(int)l.el_std_charge[l.el_ind]);
	//	cout<<l.el_std_charge[l.el_ind]<<" "<<endl;

    }

//if(l.mu_ind >-1  &&(category ==6 ||category == 7 ||(category==11 && includetHqLeptonic))) {
    if(l.mu_ind >-1  &&  (category == 6 || (category==11 && includetHqLeptonic))) {//Va SICURAMENTE CAMBIATO??  
	TLorentzVector* muon = (TLorentzVector*)l.mu_glo_p4->At(l.mu_ind);
	l.FillTree("ptMu",(float)muon->Pt());
	l.FillTree("etaMu",(float)muon->Eta());
	l.FillTree("phiMu",(float)muon->Phi());
	if((category==11 && includetHqLeptonic))	l.FillTree("chargeMu",(int)l.mu_glo_charge[l.mu_ind]);
    }
    */

    double ele_pt,mu_pt=0;

    //GIUSEPPE CONTROLLARE CHE LE CATEGORIE SIANO GIUSTE!!!!!!
    //8=> TPrime leptonic
    //9=> Tprime hadronic
    if(l.el_ind >-1) {//GIUSEPPE CONTROLLARE
	//	cout<<l.el_ind<<" ";
        TLorentzVector* electron = (TLorentzVector*)l.el_std_p4->At(l.el_ind);
	l.FillTree("ptEle",(float)electron->Pt());
	l.FillTree("etaEle",(float)electron->Eta());
	l.FillTree("phiEle",(float)electron->Phi());
	ele_pt=(double)electron->Pt();
	//	cout<<l.el_std_charge[l.el_ind]<<" "<<endl;

    }

//if(l.mu_ind >-1  &&(category ==6 ||category == 7 ||(category==11 && includetHqLeptonic))) {
    if(l.mu_ind >-1) {//GIUSEPPE CONTROLLARE  
	TLorentzVector* muon = (TLorentzVector*)l.mu_glo_p4->At(l.mu_ind);
	l.FillTree("ptMu",(float)muon->Pt());
	l.FillTree("etaMu",(float)muon->Eta());
	l.FillTree("phiMu",(float)muon->Phi());
	mu_pt=(double)muon->Pt();

    }

    /*    if(l.isLep_ele||l.isLep_mu){
	if(ele_pt>mu_pt){
	    Ht+=ele_pt;
	}else if(mu_pt>ele_pt){
	    Ht+=mu_pt;
	}
	}*/


    l.FillTree("isLep_mu",(int)l.isLep_mu);
    l.FillTree("isLep_ele",(int)l.isLep_ele);
    l.FillTree("drLepPho",(float)l.drLepPho);
    l.FillTree("drGsf",(float)l.drGsf);
    
    //l.FillTree("Ht",Ht+lead_p4.Pt()+sublead_p4.Pt()+l.met_pfmet);//CONTROLLARE
    l.FillTree("Ht",Ht);//CONTROLLARE

/*//NOTA BENE: LE CATEGORIE ADESSO SONO TUTTE CAMBIATE

*/
};


// Local Variables:
// mode: c++
// c-basic-offset: 4
// End:
// vim: tabstop=4 expandtab shiftwidth=4 softtabstop=4
