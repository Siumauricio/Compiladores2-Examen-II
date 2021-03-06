#include "ast.h"
#include <iostream>
#include <sstream>
#include <set>
#include "asm.h"
#include <map>

const char * floatTemps[] = {"$f0",
                            "$f1",
                            "$f2",
                            "$f3",
                            "$f4",
                            "$f5",
                            "$f6",
                            "$f7",
                            "$f8",
                            "$f9",
                            "$f10",
                            "$f11",
                            "$f12",
                            "$f13",
                            "$f14",
                            "$f15",
                            "$f16",
                            "$f17",
                            "$f18",
                            "$f19",
                            "$f20",
                            "$f21",
                            "$f22",
                            "$f23",
                            "$f24",
                            "$f25",
                            "$f26",
                            "$f27",
                            "$f28",
                            "$f29",
                            "$f30",
                            "$f31"
                        };

int labelCounter = 0;
#define FLOAT_TEMP_COUNT 32
set<string> intTempMap;
set<string> floatTempMap;
map<string, int> codeGenerationVars;

extern Asm assemblyFile;

int globalStackPointer = 0;

string getFloatTemp(){
    for (int i = 0; i < FLOAT_TEMP_COUNT; i++)
    {
        if(floatTempMap.find(floatTemps[i]) == floatTempMap.end()){
            floatTempMap.insert(floatTemps[i]);
            return string(floatTemps[i]);
        }
    }
    cout<<"No more float registers!"<<endl;
    return "";
}

string retrieveState(string state){
    std::string::size_type n = 0;
    string s = "sw";
    while ( ( n = state.find( s, n ) ) != std::string::npos )
    {
    state.replace( n, s.size(), "lw" );
    n += 2;
    globalStackPointer-=4;
    }
    return state;
}
string saveState(){
    set<string>::iterator it = floatTempMap.begin();
    stringstream ss;
    ss<<"sw $ra, " <<globalStackPointer<< "($sp)\n";
    globalStackPointer+=4;
    return ss.str();
}

void releaseFloatTemp(string temp){
    floatTempMap.erase(temp);
}

void FloatExpr::genCode(Code &code){
    cout<<"FloatExpr::genCode"<<endl;
    string floatTemp = getFloatTemp();
    code.place = floatTemp;
    stringstream ss;
    ss << "li.s " << floatTemp <<", "<< this->number <<endl;
    code.code = ss.str();
}

void SubExpr::genCode(Code &code){
    cout<<"SubExpr"<<endl;
   Code leftCode, rightCode;
    stringstream ss;
    this->expr1->genCode(leftCode);
    this->expr2->genCode(rightCode);
        releaseFloatTemp(leftCode.place);
        releaseFloatTemp(rightCode.place);
        ss << leftCode.code << endl
        << rightCode.code <<endl
        << "sub.s "<< leftCode.place<<", "<< leftCode.place <<", "<< rightCode.place<<endl;
        code.code = ss.str();
}

void DivExpr::genCode(Code &code){
    cout<<"DivExpr"<<endl;
    Code leftCode, rightCode;
    stringstream ss;
    this->expr1->genCode(leftCode);
    this->expr2->genCode(rightCode);
        releaseFloatTemp(leftCode.place);
        releaseFloatTemp(rightCode.place);
        ss << leftCode.code << endl
        << rightCode.code <<endl
        << "div.s "<< leftCode.place<<", "<< leftCode.place <<", "<< rightCode.place<<endl;
        code.code = ss.str();
}

void IdExpr::genCode(Code &code){
    if(floatTempMap.find(this->id) == floatTempMap.end()){
        stringstream ss;
         string floatTemp = getFloatTemp();
        code.place = floatTemp;
        ss << "l.s " << floatTemp << ", " << codeGenerationVars[this->id] << "($sp)" << endl;
        code.code = ss.str();
    }
}

string ExprStatement::genCode(){
    Code exprCode;
    this->expr->genCode(exprCode);
    releaseFloatTemp(exprCode.place);
    return exprCode.code;
}

string getNewLabel(string prefix){
    stringstream ss;
    ss<<prefix << labelCounter;
    labelCounter++;
    return ss.str();
}


string IfStatement::genCode(){
    string endIfLabel = getNewLabel("endif");
    Code exprCode;
    this->conditionalExpr->genCode(exprCode);
    stringstream ss;
    ss << exprCode.code << endl;
    ss << "bc1f" << " " << endIfLabel << endl;
    list<Statement *>::iterator itd = this->trueStatement.begin();
    while(itd != this->trueStatement.end()){
        ss << (*itd)->genCode() << endl;
        itd++;
    }
    list<Statement *>::iterator ite = this->falseStatement.begin();
    while(ite != this->falseStatement.end()){
        ss << (*ite)->genCode() << endl;
        ite++;
    }
    ss << endIfLabel << ":" << endl;
    releaseFloatTemp(exprCode.place);
    return ss.str();
}

void MethodInvocationExpr::genCode(Code &code){
    list<Expr *>::iterator it = this->expressions.begin();
    list<Code> codes;
    stringstream ss;
    Code argCode;
    while (it != this->expressions.end())

    {
        (*it)->genCode(argCode);
        ss << argCode.code <<endl;
        codes.push_back(argCode);
        it++;
    }
    int i = 0;
    list<Code>::iterator placesIt = codes.begin();
    while (placesIt != codes.end())
    {
        releaseFloatTemp((*placesIt).place);
        ss << "mfc1 $a"<<i<<", "<< (*placesIt).place<<endl;
        i++;
        placesIt++;
    }
    ss<< "jal "<< this->id<<endl;
    string reg;
    reg = getFloatTemp();
    ss << "mfc1 $v0, "<< reg<<endl;
    code.code = ss.str();
    code.place = reg;
}

string AssignationStatement::genCode(){
    Code rightCode;
    Code assignCode;
    stringstream ss;
    this->value->genCode(rightCode);
     ss << rightCode.code << endl;
    string name = this->id;
    list<Expr*>::iterator it = this->expressions.begin();
    while (it != this->expressions.end())
    {
        (*it)->genCode(assignCode);
        ss << assignCode.code << endl;
        it++;
         releaseFloatTemp(assignCode.place);

    }
    codeGenerationVars[name]=globalStackPointer;
    ss << "s.s " << rightCode.place << ", " << codeGenerationVars[name] << "($sp)" << endl;
    globalStackPointer+=4;
    releaseFloatTemp(rightCode.place);

    // stringstream ss;
    // ss << exprCode.code << endl;
    // ss << "sw " << exprCode.place << ", " << this->id << endl;
    // releaseFloatTemp(exprCode.place);
    return ss.str();
}


void GteExpr::genCode(Code &code){
    Code leftCode, rightCode;
    stringstream ss;
    this->expr1->genCode(leftCode);
    this->expr2->genCode(rightCode);
    ss << leftCode.code << endl << rightCode.code <<endl;
    releaseFloatTemp(leftCode.place);
    releaseFloatTemp(rightCode.place);
    ss<< "c.le.s "<< rightCode.place<< ", "<< leftCode.place<<endl;

    code.code = ss.str();
}

void LteExpr::genCode(Code &code){
    Code leftSideCode;
    Code rightSideCode;
    stringstream ss;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    ss << leftSideCode.code << endl << rightSideCode.code <<endl;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss << "c.lt.s " << leftSideCode.place << ", " << rightSideCode.place << endl;
    code.code = ss.str();
}

void EqExpr::genCode(Code &code){
    Code leftSideCode; 
    Code rightSideCode;
    this->expr1->genCode(leftSideCode);
    this->expr2->genCode(rightSideCode);
    stringstream ss;
    releaseFloatTemp(leftSideCode.place);
    releaseFloatTemp(rightSideCode.place);
    ss <<"c.eq.s "<< rightSideCode.code << endl << leftSideCode.code <<endl;
    code.code = ss.str();
}

void ReadFloatExpr::genCode(Code &code){
    stringstream ss;
    string reg = getFloatTemp();
    ss << "li $v0, 5" << endl;
    ss << "syscall" << endl;
    ss << "mfc1 " << reg << ", $f0" << endl;
    code.code = ss.str();
    code.place = reg;
}

string PrintStatement::genCode(){
    Code exprCode;
    stringstream asciiLabel;
    list<Expr *>::iterator it = this->expressions.begin();
    string label = getNewLabel("string");
    asciiLabel << label <<": .asciiz" << this->id << ""<<endl;
    assemblyFile.data += asciiLabel.str();

    stringstream ss;
    while (it != this->expressions.end())
    {
        (*it)->genCode(exprCode);
        ss << exprCode.code <<endl;
        releaseFloatTemp(exprCode.place);
        it++;
    }
     ss << "mov.s $f12, "<< exprCode.place<<endl
        << "li $v0, 2"<<endl
        << "syscall"<<endl;
    return ss.str();
}

string ReturnStatement::genCode(){
    Code exprCode;
    this->expr->genCode(exprCode);
    releaseFloatTemp(exprCode.place);
    stringstream ss;
    ss << exprCode.code << endl;
    ss << "mfc1 $v0, " << exprCode.place << endl;
    return ss.str();
}

string MethodDefinitionStatement::genCode(){
    cout<<"MethodDefinitionStatement"<<endl;

    if (this->params.size() > 4)
    {
        cout << "Error: Too many parameters" << endl;
        exit(1);
    }
    int stackPointer = 4;
    globalStackPointer = 0;
    stringstream code;
    code << this->id<<": "<<endl;
    string state = saveState();
    code << state << endl;

    list<string >::iterator it = this->params.begin();
    for(int i = 0; i< this->params.size(); i++){
        code << "sw $a"<<i<<", "<< stackPointer<<"($sp)"<<endl;
        stackPointer +=4;
        globalStackPointer +=4;
        it++;
    }
    
    list<Statement *>::iterator itd = this->stmts.begin();
    while(itd != this->stmts.end()){
        code << (*itd)->genCode() << endl;
        itd++;
    }

    stringstream sp;
    int currentStackPointer = globalStackPointer;

    sp << endl<<"addiu $sp, $sp, -"<<currentStackPointer<<endl;
    code << retrieveState(state);
    code << "addiu $sp, $sp, "<<currentStackPointer<<endl;
    code <<"jr $ra"<<endl;
    floatTempMap.clear();
    string result = code.str();
    result.insert(id.size() + 2, sp.str());
    return result;
}