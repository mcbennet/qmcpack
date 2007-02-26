//////////////////////////////////////////////////////////////////
// (c) Copyright 2003-  by Jeongnim Kim
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//   National Center for Supercomputing Applications &
//   Materials Computation Center
//   University of Illinois, Urbana-Champaign
//   Urbana, IL 61801
//   e-mail: jnkim@ncsa.uiuc.edu
//
// Supported by 
//   National Center for Supercomputing Applications, UIUC
//   Materials Computation Center, UIUC
//////////////////////////////////////////////////////////////////
// -*- C++ -*-
#include "QMCWaveFunctions/Jastrow/AnyConstraints.h"
#include "QMCWaveFunctions/Jastrow/LinearCombinationFunctor.h"
#include "QMCWaveFunctions/Jastrow/CompositeFunctor.h"
#include "QMCWaveFunctions/Jastrow/TwoBodyJastrowOrbital.h"
#include "QMCWaveFunctions/Jastrow/OneBodyJastrowFunction.h"
#include "Utilities/IteratorUtility.h"

namespace qmcplusplus {
  AnyConstraints::AnyConstraints(ParticleSet& p, TrialWaveFunction& psi):
    OrbitalConstraintsBase(p,psi)
    {
      //note MULTIPLE is false
      JComponent.set(ONEBODY);
      JComponent.set(TWOBODY);
    }

  AnyConstraints::~AnyConstraints() {
    //delete_iter(FuncList.begin(), FuncList.end());
  }

  bool AnyConstraints::put(xmlNodePtr cur) {
    //get generic parameters and grid information
    bool success=getVariables(cur);
    return success;
  }

  void AnyConstraints::resetParameters(OptimizableSetType& optVariables) 
  {
    cout << "What is going on " << endl;
    map<string,BasisGroupType*>::iterator it(BasisGroups.begin()),it_end(BasisGroups.end());
    while(it != it_end) {
      (*it).second->Out_->resetParameters(optVariables);
      ++it;
    }
  }

  void AnyConstraints::addOptimizables(OptimizableSetType& outVars) 
  {
    map<string,BasisGroupType*>::iterator it(BasisGroups.begin()),it_end(BasisGroups.end());
    while(it != it_end) {
      (*it).second->In_->addOptimizables(outVars);
      ++it;
    }
  }

  void AnyConstraints::addSingleBasisPerSpecies(xmlNodePtr cur) 
  {

    RealType rcut=10.0;
    int npts=101;
    RealType step=-1.0;
    if(myGrid) {
      rcut=myGrid->rmax();
      npts = myGrid->size();
    }

    OhmmsAttributeSet gAttrib;
    gAttrib.add(rcut,"rf");
    gAttrib.add(npts,"npts");
    gAttrib.add(step,"step");

    BasisGroupType* curBG=0;
    cur=cur->children;
    while(cur != NULL)
    {
      string cname((const char*)(cur->name));
      string elementType("e");
      OhmmsAttributeSet aAttrib;
      aAttrib.add(elementType,"elementType");
      aAttrib.put(cur);
      if(cname == "atomicBasisSet")
      {
        xmlNodePtr cur1=cur->children;
        while(cur1 != NULL)
        {
          string cname1((const char*)(cur1->name));
          if(cname1 == "basisGroup")
          {
            curBG=createBasisGroup(cur1,elementType);
            curBG->setGrid(rcut,npts);
            add2BasisGroup(curBG,cur1);
          }
          else if(cname1 == "grid")
            gAttrib.put(cur1);
          cur1=cur1->next;
        }
      }
      else if(cname == "basisGroup")
      {
        curBG=createBasisGroup(cur,elementType);
        curBG->setGrid(rcut,npts);
        add2BasisGroup(curBG,cur);
      }
      else if(cname == "grid")
        gAttrib.put(cur);
      cur=cur->next; 
    }
  }


  AnyConstraints::BasisGroupType* 
    AnyConstraints::createBasisGroup(xmlNodePtr cur, const string& elementType)
 {
    string type("WM");

    OhmmsAttributeSet aAttrib;
    aAttrib.add(type,"type");
    aAttrib.put(cur);

    BGContainerType::iterator it(BasisGroups.find(elementType));
    BasisGroupType* curBG=0;
    if(it == BasisGroups.end()) 
    {
      curBG=new BasisGroupType;
      BasisGroups[elementType]=curBG;
    }
    else 
    {
      curBG=(*it).second;
    }

    return curBG;
 }

  void AnyConstraints::add2BasisGroup(BasisGroupType* curBG, xmlNodePtr cur)
  {
    typedef LinearCombinationFunctor<RealType> ComboFuncType;
    ComboFuncType* acombo=new ComboFuncType;
    curBG->In_ = acombo; 

    cur=cur->children;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == "radfunc") {
        OhmmsAttributeSet rAttrib;
        string radID("0");
        string rfuncName("WM");
        RealType exponent=1.0;
        RealType contraction=1.0;
        RealType rcut(curBG->Rcut);
        int rpower=0;
        rAttrib.add(radID,"id"); //rAttrib.add(a->B0,"b");
        rAttrib.add(exponent,"exponent"); 
        rAttrib.add(contraction,"contraction"); 
        rAttrib.add(rpower,"node"); 
        rAttrib.add(rcut,"rcut"); 
        rAttrib.add(rfuncName,"type"); 
        rAttrib.put(cur);

        OptimizableFunctorBase<RealType>* a=0;
        if(rfuncName == "cusp")
        {
          a= new CuspCorrectionFunctor<RealType>(exponent,rcut);
          rpower=0;//overwrite the power
        }
        else
        {
          a= new WMFunctor<RealType>(exponent,rcut);
        }
        a->put(cur);
        string id_c=radID+"_C";
        if(rpower == 0)
        {
          acombo->addComponent(a,contraction,id_c,cur);
        }
        else
        {
          AnyTimesRnFunctor<RealType>* awrap=new AnyTimesRnFunctor<RealType>(a,rpower);
          acombo->addComponent(awrap,contraction,id_c,cur);
        }
        app_log()  << "<radfunc id=\"" << radID << "\" exponent=\""<< exponent 
          << "\" contraction=\"" << contraction 
          << "\" node=\"" << rpower << "\" rcut=\"" << rcut << "\"/>" << endl;
      }
      cur=cur->next;
    }
 }

  OrbitalBase* AnyConstraints::createTwoBody() {
    xmlNodePtr cur=myNode->children;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == "basisset")
      {
        addSingleBasisPerSpecies(cur);
      }
      cur=cur->next;
    }

    if(BasisGroups.empty())
    {
      app_error() << "  AnyConstraints::createTwoBody fails to create a TwoBodyJastrow "
        << " due to missing <basisset/> " << endl;
      return 0;
    }

    BasisGroupType* curGroup=0;
   
    BGContainerType::iterator it(BasisGroups.find(targetPtcl.getName()));
    if(it == BasisGroups.end())
    {
      return 0;
    }
    else
    {
      curGroup=(*it).second;
    }

    InFuncType* infunc=curGroup->In_;
    OutFuncType* nfunc= new OutFuncType(infunc, curGroup->Rcut, curGroup->NumGridPoints);
    curGroup->Out_ = nfunc;
#if !defined(HAVE_MPI)
    if(PrintTables) {
      ofstream fout("J2.dat");
      fout.setf(ios::scientific, ios::floatfield);
      fout << "# Two-body Jastrow generated by AnyContraints::createTwoBody" << endl;
      nfunc->print(fout);
    }
#endif
    //create a Jastrow function
    typedef TwoBodyJastrowOrbital<OutFuncType> JeeType;
    JeeType *J2 = new JeeType(targetPtcl);
    J2->insert("Jee",nfunc);
    for(int i=0; i<4; i++) J2->addFunc(nfunc);
    return J2;
  }

  OrbitalBase* AnyConstraints::createOneBody(ParticleSet& source) {
    map<string,InFuncType*> jnSet;
    xmlNodePtr cur=myNode->children;
    while(cur != NULL) {
      string cname((const char*)(cur->name));
      if(cname == "basisset")
      {
        addSingleBasisPerSpecies(cur);
      }
      cur=cur->next;
    }

    int nSpecies = source.getSpeciesSet().getTotalNum();

    typedef OneBodyJastrow<OutFuncType> JneType;
    JneType* jne=new JneType(source,targetPtcl);

    BGContainerType::iterator jit(BasisGroups.begin()), jit_end(BasisGroups.end());
    while(jit != jit_end)
    {
      int ig=source.getSpeciesSet().findSpecies((*jit).first);
      if(ig < nSpecies) //should not add any species here
      {
        BasisGroupType* curG((*jit).second);
        OutFuncType* nfunc= new OutFuncType(curG->In_,curG->Rcut, curG->NumGridPoints);
        jne->addFunc(ig,nfunc);
        curG->Out_ = nfunc;
#if !defined(HAVE_MPI)
        if(PrintTables) {
          char fname[16];
          sprintf(fname,"J1.%s.dat",source.getSpeciesSet().speciesName[ig].c_str());
          ofstream fout(fname);
          fout.setf(ios::scientific, ios::floatfield);
          fout << "# One-body Jastrow " << source.getSpeciesSet().speciesName[ig] 
            << " generated by AnyContraints::createOneBody" << endl;
          nfunc->print(fout);
        }
#endif
      }
      ++jit;
    }

    return jne;
  }


 // void
 //   AnyConstraints::createDefaultTwoBody(xmlNodePtr cur, const string& tname) {
 //    //xmlNodePtr basisNode = xmlNewNode(NULL,(const xmlChar*)"basisGroup");
 //    //xmlNewProp(basisNode,(const xmlChar*)"source",(const xmlChar*)tname.c_str());
 //    //xmlNodePtr b0 = xmlNewTextChild(basisNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"4.55682");
 //    //xmlNewProp(b0,(const xmlChar*)"id",(const xmlChar*)"eeB0");
 //    //xmlNewProp(b0,(const xmlChar*)"name",(const xmlChar*)"B");
 //    //xmlNodePtr b1 = xmlNewTextChild(basisNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"1.83679");
 //    //xmlNewProp(b1,(const xmlChar*)"id",(const xmlChar*)"eeB1");
 //    //xmlNewProp(b1,(const xmlChar*)"name",(const xmlChar*)"B");
 //    //xmlNodePtr b2 = xmlNewTextChild(basisNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"4.59201");
 //    //xmlNewProp(b2,(const xmlChar*)"id",(const xmlChar*)"eeB2");
 //    //xmlNewProp(b2,(const xmlChar*)"name",(const xmlChar*)"B");
 //    //xmlNodePtr b3 = xmlNewTextChild(basisNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"0.226152");
 //    //xmlNewProp(b3,(const xmlChar*)"id",(const xmlChar*)"eeB3");
 //    //xmlNewProp(b3,(const xmlChar*)"name",(const xmlChar*)"B");
 //    //xmlAddChild(cur,basisNode);

 //    //addBasisGroup(basisNode);

 //    //xmlNodePtr corrNode = xmlNewNode(NULL,(const xmlChar*)"correlation");
 //    //xmlNewProp(corrNode,(const xmlChar*)"speciesA",(const xmlChar*)tname.c_str());
 //    //xmlNewProp(corrNode,(const xmlChar*)"speciesB",(const xmlChar*)tname.c_str());
 //    //xmlNodePtr c0 = xmlNewTextChild(corrNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"-0.137958");
 //    //xmlNewProp(c0,(const xmlChar*)"id",(const xmlChar*)"eeC0");
 //    //xmlNewProp(c0,(const xmlChar*)"name",(const xmlChar*)"C");
 //    //xmlNodePtr c1 = xmlNewTextChild(corrNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"-1.09405");
 //    //xmlNewProp(c1,(const xmlChar*)"id",(const xmlChar*)"eeC1");
 //    //xmlNewProp(c1,(const xmlChar*)"name",(const xmlChar*)"C");
 //    //xmlNodePtr c2 = xmlNewTextChild(corrNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"1.06578");
 //    //xmlNewProp(c2,(const xmlChar*)"id",(const xmlChar*)"eeC2");
 //    //xmlNewProp(c2,(const xmlChar*)"name",(const xmlChar*)"C");
 //    //xmlNodePtr c3 = xmlNewTextChild(corrNode,NULL,(const xmlChar*)"parameter",(const xmlChar*)"0.96602");
 //    //xmlNewProp(c3,(const xmlChar*)"id",(const xmlChar*)"eeC3");
 //    //xmlNewProp(c3,(const xmlChar*)"name",(const xmlChar*)"C");
 //    //xmlAddChild(cur,corrNode);

 //    //map<string,BasisSetType*>::iterator it(myBasisSet.find("e"));
 //    //return createCorrelation(corrNode,(*it).second);
 // }
}
/***************************************************************************
 * $RCSfile$   $Author: jnkim $
 * $Revision: 1693 $   $Date: 2007-02-02 12:13:11 -0600 (Fri, 02 Feb 2007) $
 * $Id: AnyConstraints.cpp 1693 2007-02-02 18:13:11Z jnkim $ 
 */
