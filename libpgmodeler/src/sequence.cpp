#include "sequence.h"

const QString Sequence::VALOR_MAX_POSITIVO="+9223372036854775807";
const QString Sequence::VALOR_MAX_NEGATIVO="-9223372036854775808";

Sequence::Sequence(void)
{
 obj_type=OBJ_SEQUENCE;
 ciclica=false;
 incremento=inicio=cache="1";
 valor_min="0";
 valor_max=VALOR_MAX_POSITIVO;
 coluna=NULL;

 attributes[ParsersAttributes::INCREMENT]="";
 attributes[ParsersAttributes::MIN_VALUE]="";
 attributes[ParsersAttributes::MAX_VALUE]="";
 attributes[ParsersAttributes::START]="";
 attributes[ParsersAttributes::CACHE]="";
 attributes[ParsersAttributes::CYCLE]="";
 attributes[ParsersAttributes::OWNER_COLUMN]="";
}

bool Sequence::valorNulo(const QString &valor)
{
 unsigned i, qtd;
 bool nulo;

 i=0;
 nulo=true;
 qtd=valor.size();
 while(i < qtd && nulo)
 {
  nulo=(valor[i]=='0' || valor[i]=='+' || valor[i]=='-');
  i++;
 }
 return(nulo);
}

bool Sequence::valorValido(const QString &valor)
{
 /*
  Para que um valor seja válido o mesmo deve ou não iniciar com
  operador + ou - ser constituído apenas de números. E o seu
  tamanho não deve ultrapassar o tamanho da constante VALOR_MAX_POSITIVO
 */
 if(valor.size() > VALOR_MAX_POSITIVO.size())
  return(false);
 else
 {
  unsigned i, qtd;
  bool oper=false, num=false, valido=true;

  qtd=valor.size();
  for(i=0; i < qtd && valido; i++)
  {
   if((valor[i]=='-' || valor[i]=='+') && !num)
   {
    if(!oper) oper=true;
   }
   else if((valor[i]>='0' && valor[i]<='9'))
   {
    if(!num) num=true;
   }
   else valido=false;
  }

  if(!num) valido=false;
  return(valido);
 }
}

QString Sequence::formatarValor(const QString &valor)
{
 QString valor_fmt;

 //Verifica se o valor é válido
 if(valorValido(valor))
 {
  unsigned i, qtd, qtd_neg;

  i=qtd_neg=0;
  qtd=valor.size();
  /* Conta a quantidade de operadores negativo, pois
     dependendo da quantidade o mesmo pode interferir
     no sinal do número */
  while((valor[i]=='+' || valor[i]=='-') && i < qtd)
  {
   if(valor[i]=='-') qtd_neg++;
   i++;
  }

  //Caso a quantidade de negativos seja ímpar o número será negativo
  if(qtd_neg % 2 != 0) valor_fmt+="-";
  //valor_fmt+=valor.substr(i, qtd);
  valor_fmt+=valor.mid(i, qtd);
 }

 return(valor_fmt);
}

int Sequence::compararValores(QString valor1, QString valor2)
{
 if(valor1==valor2)
  return(0);
 else
 {
  char ops[2];
  unsigned i, idx, qtd;
  QString *vet_valores[2]={&valor1, &valor2}, valor_aux;

  for(i=0; i < 2; i++)
  {
   //Obtém o sinal do número
   ops[i]=vet_valores[i]->at(0).toAscii();
   //Caso não possua sinal, um + será adicionado
   if(ops[i]!='-' && ops[i]!='+') ops[i]='+';

   //Obtém o restante do número sem o sinal
   idx=1;
   qtd=vet_valores[i]->size();
   while(idx < qtd)
   {
    if(vet_valores[i]->at(idx)!='0')
    valor_aux+=vet_valores[i]->at(idx);
    idx++;
   }
   (*vet_valores[i])=valor_aux;
   valor_aux="";
  }

  //Compara os sinais e os valores, caso sejam iguais retorna 0
  if(ops[0]==ops[1] && valor1==valor2)
   return(0);
  /* Caso os operadores sejam iguais e o valor1 for menor que o valor2 ou
     se os sinais sejam diferentes */
  else if((ops[0]=='-' && ops[1]=='-' && valor1 > valor2) ||
          (ops[0]=='+' && ops[1]=='+' && valor1 < valor2) ||
          (ops[0]=='-' && ops[1]=='+'))
   //Retorna -1 indicando que o valor 1 é menor que o valor 2
   return(-1);
  else
   //Retorna 1 indicando que o valor2 é maior que valor1
   return(1);
 }
}

void Sequence::setName(const QString &nome)
{
 QString nome_ant=this->getName(true);
 BaseObject::setName(nome);

 /* Renomeia o tipo já definido anteriormente na
    lista de tipos do PostgreSQL */
 TipoPgSQL::renomearTipoUsuario(nome_ant, this, this->getName(true));
}

void Sequence::setSchema(BaseObject *esquema)
{
 Tabela *tabela=NULL;
 QString nome_ant=this->getName(true);

 //Caso a coluna possuidora da sequencia exista
 if(coluna)
 {
  //Obtém a tabela pai da coluna
  tabela=dynamic_cast<Tabela *>(coluna->getParentTable());

  //Verifica se o esquema sendo atribuíd  seqüência é o mesmo da tabela possuidora
  if(tabela && tabela->getSchema()!=esquema)
    throw Exception(ERR_ASG_SEQ_DIF_TABLE_SCHEMA,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 }

 //Atribui o esquema   sequencia
 BaseObject::setSchema(esquema);

 /* Renomeia o tipo já definido anteriormente na
    lista de tipos do PostgreSQL */
 TipoPgSQL::renomearTipoUsuario(nome_ant, this, this->getName(true));
}

void Sequence::definirCiclica(bool valor)
{
 ciclica=valor;
}

void Sequence::definirValores(QString vmin, QString vmax, QString inc, QString inicio, QString cache)
{
 vmin=formatarValor(vmin);
 vmax=formatarValor(vmax);
 inc=formatarValor(inc);
 inicio=formatarValor(inicio);
 cache=formatarValor(cache);

 /* Caso algum atributo após a formatação esteja vazio quer dizer
    que seu valor é invalido, sendo assim uma exceção é disparada*/
 if(vmin==""   || vmax=="" || inc=="" ||
    inicio=="" || cache=="")
  throw Exception(ERR_ASG_INV_VALUE_SEQ_ATTRIBS,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 else if(compararValores(vmin,vmax) > 0)
  throw Exception(ERR_ASG_INV_SEQ_MIN_VALUE,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 else if(compararValores(inicio, vmin) < 0 ||
         compararValores(inicio, vmax) > 0)
  throw Exception(ERR_ASG_INV_SEQ_START_VALUE,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 else if(valorNulo(inc))
  throw Exception(ERR_ASG_INV_SEQ_INCR_VALUE,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 else if(valorNulo(cache))
  throw Exception(ERR_ASG_INV_SEQ_CACHE_VALUE,__PRETTY_FUNCTION__,__FILE__,__LINE__);

 this->valor_min=vmin;
 this->valor_max=vmax;
 this->incremento=inc;
 this->cache=cache;
 this->inicio=inicio;
}

void Sequence::definirPossuidora(Tabela *tabela, const QString &nome_coluna)
{
 if(!tabela || nome_coluna=="")
  this->coluna=NULL;
 else if(tabela)
 {
  // Verifica se a tabela não pertence ao mesmo esquema da sequencia.
  //   Caso não pertença, dispara uma exceção.
  if(tabela->getSchema()!=this->schema)
   throw Exception(Exception::getErrorMessage(ERR_ASG_TAB_DIF_SEQ_SCHEMA)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_TAB_DIF_SEQ_SCHEMA,__PRETTY_FUNCTION__,__FILE__,__LINE__);

    /* Verifica se a tabela não pertence ao mesmo dono da sequencia.
     Caso não pertença, dispara uma exceção. */
  if(tabela->getOwner()!=this->owner)
   throw Exception(Exception::getErrorMessage(ERR_ASG_SEQ_OWNER_DIF_TABLE)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_SEQ_OWNER_DIF_TABLE,__PRETTY_FUNCTION__,__FILE__,__LINE__);

  //Obtém a coluna da tabela com base no nome passado
  this->coluna=tabela->obterColuna(nome_coluna);

  if(this->coluna && this->coluna->isAddedByRelationship() &&
     this->coluna->getObjectId() > this->object_id)
   this->object_id=BaseObject::getGlobalId();


  //Caso a coluna não exista
  if(!this->coluna)
   throw Exception(Exception::getErrorMessage(ERR_ASG_INEXIST_OWNER_COL_SEQ)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_INEXIST_OWNER_COL_SEQ,__PRETTY_FUNCTION__,__FILE__,__LINE__);
 }
}

void Sequence::definirPossuidora(Column *coluna)
{
 Tabela *tabela=NULL;

 if(!coluna)
  this->coluna=NULL;
 else
 {
  tabela=dynamic_cast<Tabela *>(coluna->getParentTable());

  //CAso a coluna possuidor não seja de uma tabela
  if(!tabela)
   throw Exception(Exception::getErrorMessage(ERR_ASG_INV_OWNER_COL_SEQ)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_INV_OWNER_COL_SEQ,__PRETTY_FUNCTION__,__FILE__,__LINE__);

  /* Verifica se a tabela não pertence ao mesmo esquema da sequencia.
     Caso não pertença, dispara uma exceção. */
  if(tabela->getSchema()!=this->schema)
   throw Exception(Exception::getErrorMessage(ERR_ASG_TAB_DIF_SEQ_SCHEMA)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_TAB_DIF_SEQ_SCHEMA,__PRETTY_FUNCTION__,__FILE__,__LINE__);

  /* Verifica se a tabela não pertence ao mesmo dono da sequencia.
     Caso não pertença, dispara uma exceção. */
  if(tabela->getOwner()!=this->owner)
   throw Exception(Exception::getErrorMessage(ERR_ASG_SEQ_OWNER_DIF_TABLE)
                 .arg(QString::fromUtf8(this->getName(true))),
                 ERR_ASG_SEQ_OWNER_DIF_TABLE,__PRETTY_FUNCTION__,__FILE__,__LINE__);

  this->coluna=coluna;

  if(coluna && coluna->isAddedByRelationship() &&
     coluna->getObjectId() > this->object_id)
   this->object_id=BaseObject::getGlobalId();
 }
}

bool Sequence::referenciaColunaIncRelacao(void)
{
 return(coluna && coluna->isAddedByRelationship());
}

bool Sequence::sequenciaCiclica(void)
{
 return(ciclica);
}

QString Sequence::obterValorMax(void)
{
 return(valor_max);
}

QString Sequence::obterValorMin(void)
{
 return(valor_min);
}

QString Sequence::obterCache(void)
{
 return(cache);
}

QString Sequence::obterIncremento(void)
{
 return(incremento);
}

QString Sequence::obterInicio(void)
{
 return(inicio);
}

Column *Sequence::obterPossuidora(void)
{
 return(coluna);
}

QString Sequence::getCodeDefinition(unsigned tipo_def)
{
 QString str_aux;
 Tabela *tabela=NULL;

 //Caso haja uma coluna possuidora
 if(coluna)
 {
  tabela=dynamic_cast<Tabela *>(coluna->getParentTable());
  /* Formata o atributo possuidora como sendo o nome da tabela
     e a coluna possuidora */
  str_aux=tabela->getName(true) + "." + coluna->getName(true);
 }
 attributes[ParsersAttributes::OWNER_COLUMN]=str_aux;

 attributes[ParsersAttributes::INCREMENT]=incremento;
 attributes[ParsersAttributes::MIN_VALUE]=valor_min;
 attributes[ParsersAttributes::MAX_VALUE]=valor_max;
 attributes[ParsersAttributes::START]=inicio;
 attributes[ParsersAttributes::CACHE]=cache;
 attributes[ParsersAttributes::CYCLE]=(ciclica ? "1" : "");

 return(BaseObject::__getCodeDefinition(tipo_def));
}

void Sequence::operator = (Sequence &seq)
{
 QString nome_ant=this->getName(true);

 *(dynamic_cast<BaseObject *>(this))=dynamic_cast<BaseObject &>(seq);

 this->ciclica=seq.ciclica;
 this->valor_max=seq.valor_max;
 this->valor_min=seq.valor_min;
 this->inicio=seq.inicio;
 this->incremento=seq.incremento;
 this->cache=seq.cache;
 this->coluna=seq.coluna;

 TipoPgSQL::renomearTipoUsuario(nome_ant, this, this->getName(true));
}
