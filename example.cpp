#include <bits/stdc++.h>
using namespace std;

#include "gurobi_c++.h"

struct Modele {
    GRBEnv* env;
    GRBModel* model;

    GRBConstr max_boites;
    vector<GRBConstr> max_objets;
    vector<int> maxDispos;
    vector<double> profits;
    vector<int> taillesObjets;
    int tailleBoite;

    Modele(int nbBoites, vector<int> maxDispos_, vector<double> profits_, vector<int> taillesObjets_, int tailleBoite_)
        : profits(profits_), maxDispos(maxDispos_), taillesObjets(taillesObjets_), tailleBoite(tailleBoite_) {
        env = new GRBEnv(true);
        env->set(GRB_IntParam_LogToConsole, 0);
        env->start();

        model = new GRBModel(*env);

        max_boites = model->addConstr(0.0, GRB_LESS_EQUAL, nbBoites);

        for (int iObjet = 0; iObjet < (int)maxDispos.size(); iObjet++) {
            max_objets.push_back(model->addConstr(0.0, GRB_LESS_EQUAL, maxDispos[iObjet]));
        }

        model->optimize();
    }

    vector<double> dual_des_bornes() {
        vector<double> duaux;

        for (auto& contrainte : max_objets) {
            duaux.push_back(contrainte.get(GRB_DoubleAttr_Pi));
        }

        return duaux;
    }

    double dual_des_boites() {
        return max_boites.get(GRB_DoubleAttr_Pi);
    }

    void ajoute_colonne(vector<int> nombre_fois) {
        double profit = 0;
        for (int iObjet = 0; iObjet < (int)nombre_fois.size(); iObjet++) {
            profit += nombre_fois[iObjet] * profits[iObjet];
        }

        GRBColumn col;
        col.addTerm(1, max_boites);

        for (int iObjet = 0; iObjet < (int)nombre_fois.size(); iObjet++) {
            col.addTerm(nombre_fois[iObjet], max_objets[iObjet]);
        }

        model->addVar(0, INFINITY, -profit, GRB_CONTINUOUS, col);
    }

    void optimize() {
        model->set(GRB_IntParam_Method, 0);
        model->optimize();
    }

    double obj() {
        return -model->get(GRB_DoubleAttr_ObjVal);
    }

    vector<double> profits_reduits() {
        auto duaux = dual_des_bornes();
        vector<double> resultat;
        for (int iObjet = 0; iObjet < duaux.size(); iObjet++) {
            resultat.push_back(profits[iObjet] + duaux[iObjet]);
        }
        return resultat;
    }

    double theta() {
        return -max_boites.get(GRB_DoubleAttr_Pi);
    }

    vector<int> pricing() {
        GRBModel pricing_model(env);

        auto preduits = profits_reduits();

        vector<GRBVar> nbOccs;
        GRBLinExpr constr;

        for (int iObjet = 0; iObjet < (int)profits.size(); iObjet++) {
            nbOccs.push_back(pricing_model.addVar(0.0, maxDispos[iObjet], -preduits[iObjet], GRB_INTEGER));
            constr += taillesObjets[iObjet] * nbOccs.back();
        }

        pricing_model.addConstr(constr <= tailleBoite);

        pricing_model.optimize();

        double robj = -pricing_model.get(GRB_DoubleAttr_ObjVal);

        if (robj <= theta() + 1e-6) return {};

        vector<int> col;
        for (int iObjet = 0; iObjet < (int)profits.size(); iObjet++) {
            col.push_back(nbOccs[iObjet].get(GRB_DoubleAttr_X));
        }

        return col;
    }

    void generation_colonnes() {
        while (true) {
            auto col = pricing();
            if (col.empty()) break;
            ajoute_colonne(col);
            optimize();
        }
    }

    ~Modele() {
        delete env;
        delete model;
    }
};

int main(int argc, char** argv) {
    Modele m(3, {1, 2, 3}, {2.0, 3.0, 1.0}, {3, 5, 1}, 10);

    m.generation_colonnes();

    cout << "relaxation LP maitre : " << m.obj() << endl;

    return 0;
}