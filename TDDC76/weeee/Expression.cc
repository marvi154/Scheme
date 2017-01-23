/*
 * FILNAMN:       Expression.cc
 * LABORATION:    lab3
 * PROGRAMMERARE: Victor Viberg 911211-4716
 *                Christian W�ngblad 870323-7118
 * DATUM:         2012-10-17
 *
 * BESKRIVNING 
 * 
 * Skapar Expression klassen, och hanterar infix och postfix
 */
#include "Expression.h"
#include "Expression_Tree.h"
#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>
using namespace std;



double Expression::evaluate() const
{
  if (empty())
    {
      throw Expression_Tree_Error("Inget uttryck");
      return 0;
    }
  else 
    return tree->evaluate(); 
}


std::string Expression::get_postfix() const
{
  if (empty())
    throw Expression_Tree_Error("Inget uttryck");
 
  else 
    {
      tree->evaluate(); // get_postfix ska kasta undantag i samma fall som evaluate
      return tree->get_postfix();
    }
}


bool Expression::empty() const
{
  return (tree == 0);
}


void Expression::print_tree(std::ostream&) const
{
  if(empty())
    throw Expression_Tree_Error("Inget uttryck");
  else 
    {
      tree->evaluate(); //print_tree  ska kasta undantag i samma fall som evaluate
      tree->print(cout,2);
    }
}


void Expression::swap(Expression& n)
{
  Expression_Tree* temp = tree;
  tree = n.tree;
  n.tree = temp;
}


void swap(Expression& expression1, Expression& expression2)
{
  Expression& temp = expression1;
  expression2.swap(expression1);
  expression1.swap(temp);
}

/*
 * make_expression() definieras efter namnrymden nedan.
 */

// Namnrymden nedan inneh�ller intern kod f�r infix-till-postfix-omvandling
// och generering av uttryckstr�d. Anv�nds endast i denna fil.
namespace
{
   using std::vector;
   using std::map;
   using std::make_pair;
   using std::string;

   // Underlag f�r att skapa prioritetstabellerna, mm. H�gre v�rde inom
   // input_prio respektive stack_prio anger inb�rdes prioritetsordning.
   // H�gre v�rde i input_prio j�mf�rt med motsvarande position i stack_prio
   // inneb�r h�gerassociativitet, det motsatta v�nsterassociativitet.
   // (Flerteckenoperatorer kan ocks� hanteras med denna representation)
   const char* ops[]      = { "^", "*", "/", "+", "-", "=" };
   const int input_prio[] = {  8,   5,   5,   3,   3,   2  };
   const int stack_prio[] = {  7,   6,   6,   4,   4,   1  };
   const int n_ops = sizeof ops / sizeof(char*);

   // Till�tna operatorer. Anv�nds av make_postfix() och make_expression_tree().
   const vector<string> operators(ops, ops + n_ops);

   // Teckenupps�ttningar f�r operander. Anv�nds av make_expression_tree().
   const string letters("abcdefghijklmnopqrstuvwxyz");
   const string digits("0123456789");
   const string integer_chars(digits);
   const string real_chars(digits + '.');
   const string variable_chars(letters);
   const string operand_chars(letters + digits + '.');

   // Hj�lpfunktioner f�r att kategorisera lexikala element.
   bool is_operator(char token)
   {
      return find(operators.begin(), operators.end(), string(1, token)) != operators.end();
   }

   bool is_operator(const string& token)
   {
      return find(operators.begin(), operators.end(), token) != operators.end();
   }

   bool is_operand(const string& token)
   {
      return token.find_first_not_of(operand_chars) == string::npos;
   }

   bool is_integer(const string& token)
   {
      return token.find_first_not_of(integer_chars) == string::npos;
   }

   bool is_real(const string& token)
   {
      return token.find_first_not_of(real_chars) == string::npos;
   }

   bool is_identifier(const string& token)
   {
      return token.find_first_not_of(letters) == string::npos;
   }

   // Prioritetstabeller. Anv�nds av make_postfix().
   typedef map<string, int> priority_table;

   // Funktion f�r att initiera en prioritetstabell, givet de till�tna operatorerna,
   // deras prioritieter (inkommandeprioritet eller stackprioritet, beroende p� vilken
   // tabell som ska skapas) och antalet operatorer.
   priority_table init_priority_table(const char* ops[], const int* prio, const int n_ops)
   {
      priority_table pm;

      for (int i = 0; i < n_ops; ++i)
      {
	 pm.insert(make_pair(string(ops[i]), prio[i]));
      }

      return pm;
   }

   // Prioritetstabellerna, en f�r inkommandeprioritet och en f�r stackprioritet.
   const priority_table input_priority = init_priority_table(ops, input_prio, n_ops);
   const priority_table stack_priority = init_priority_table(ops, stack_prio, n_ops);

  // format_infix() tar en str�ng med ett infixuttryck och formaterar den s� att
  // det finns ett mellanrum mellan varje symbol. Anv�nds av make_postfix(), d�r
  // denna formatering underl�ttar parsningen.

   std::string format_infix(const std::string& infix)
   {
      string::const_iterator bos = infix.begin();
      string::const_iterator eos = infix.end();
      string                 formated;

      for (string::const_iterator it = bos; it != eos; ++it)
      {
	 if (is_operator(*it) || *it == '(' || *it == ')')
	 {
	    // Se till att det �r ett mellanrum f�re en operator eller parentes
	    if (it != bos && *(it - 1) != ' ' && *(formated.end() - 1) != ' ')
	       formated.append(1, ' ');
	    formated.append(1, *it);
	    // Se till att det �r ett mellanrum efter en operator eller parentes
	    if ((it + 1) != eos && *(it + 1) != ' ')
	       formated.append(1, ' ');
	 }
	 else
	 {
	    if (*it != ' ')
	       formated.append(1, *it);
	    else if (it != bos && *(it - 1) != ' ')
	       formated.append(1, *it);
	 }
      }
      return formated;
   }

   // make_postfix() tar en infixstr�ng och returnerar motsvarande postfixstr�ng.

   std::string make_postfix(const std::string& infix)
   {
      using std::stack;
      using std::string;
      using std::istringstream;
      using std::find;

      stack<string> operator_stack;
      string        token;
      string        previous_token = "";
      bool          last_was_operand = false;
      int           paren_count = 0;

      istringstream is(format_infix(infix));
      string        postfix;

      while (is >> token)
      {
	 if (is_operator(token))
	 {
	    if (! last_was_operand || postfix.empty() || previous_token == "(")
	    {

	       throw Expression_Error("operator d�r operand f�rv�ntades\n");
	    }

	    // J�rnv�gsalgoritmen
	    while (! operator_stack.empty() &&
		   input_priority.find(token)->second <=
		   stack_priority.find(operator_stack.top())->second)
	      {
		postfix += operator_stack.top() + ' ';
		operator_stack.pop();
	      }
	    operator_stack.push(token);
	    last_was_operand = false;
	 }
	 else if (token == "(")
	   {
	     operator_stack.push(token);
	     ++paren_count;
	   }
	 else if (token == ")")
	   {
	     if (paren_count == 0)
	       {
		 throw Expression_Error("v�nsterparentes saknas\n");
	       }

	     if (previous_token == "(" && ! postfix.empty())
	       {
		 std::cerr << "tom parentes\n";
		 exit(EXIT_FAILURE);
	       }

	     while (! operator_stack.empty() && operator_stack.top() != "(")
	       {
		 postfix += operator_stack.top() + ' ';
		 operator_stack.pop();
	       }

	     if (operator_stack.empty())
	       {
		 throw Expression_Error("h�gerparentes saknar matchande v�nsterparentes\n");

	       }
	 // Det finns en v�nsterparentes p� stacken
	 operator_stack.pop();
	    --paren_count;
	 }
	 else if (is_operand(token))
	 {
	    if (last_was_operand || previous_token == ")")
	    {
	       std::cerr << "operand d�r operator f�rv�ntades\n";
	       exit(EXIT_FAILURE);
	    }

	    postfix += token + ' ';
	    last_was_operand = true;
	 }
	 else
	   {
	     throw Expression_Error("otill�ten symbol\n");

	   }

	 previous_token = token;
      }

      if (postfix == "")
	{
	  throw Expression_Error("tomt infix\n");

	}

      if ((! last_was_operand) && (! postfix.empty()))
      {
	  throw Expression_Error("operatorn avslutar\n");

      }

      if (paren_count > 0)
	{
	  throw Expression_Error("h�gerparentes saknas\n");

	}

      while (! operator_stack.empty())
      {
	 postfix += operator_stack.top() + ' ';
	 operator_stack.pop();
      }

      if (! postfix.empty())
      {
	 postfix.erase(postfix.size() - 1);
      }

      return postfix;
   }

  void clearmemory(stack<Expression_Tree*> n) //tar bort allt allokerat minne f�r objekten i stacken
  {
    while (! n.empty())
      {
	delete n.top();
	n.pop();
      }
  }

  // make_expression_tree() tar en postfixstr�ng och returnerar ett motsvarande 
  // l�nkat tr�d av Expression_Tree-noder. T�nk p� minnesl�ckage...

  Expression_Tree* make_expression_tree(const std::string& postfix)
  {
    using std::stack;
    using std::string;
    using std::istringstream;

    stack<Expression_Tree*> tree_stack;
    string                  token;
    istringstream           ps(postfix);

    while (ps >> token)
      {
	try
	  {
	    if (is_operator(token))
	      {
		if (tree_stack.empty()) 
		  {
		    throw Expression_Error("felaktigt postfix\n");

		  }
		Expression_Tree* rhs = tree_stack.top();
		tree_stack.pop();

		if (tree_stack.empty()) 
		  {
		    throw Expression_Error("felaktigt postfix\n");

		  }
		Expression_Tree* lhs = tree_stack.top();
		tree_stack.pop();
		try 
		  {
		    if (token == "^")
		      {
			tree_stack.push(new Power(lhs, rhs));
		      }
		    else if (token == "*")
		      {
			tree_stack.push(new Times(lhs, rhs));
		      }
		    else if (token == "/")
		      {
			tree_stack.push(new Divide(lhs, rhs));
		      }
		    else if (token == "+")
		      {
	
			tree_stack.push(new Plus(lhs, rhs));
		      }
		    else if (token == "-")
		      {
			tree_stack.push(new Minus(lhs, rhs));
		      }
		    else if (token == "=")
		      {	
			tree_stack.push(new Assign(lhs, rhs));
		      }
		  }
		catch(...)
		  {
		    delete rhs;
		    delete lhs;
		    throw;
		  }
	      }
	    else if (is_integer(token))
	      {
		tree_stack.push(new Integer(atoi(token.c_str())));
	      }
	    else if (is_real(token))
	      {
		tree_stack.push(new Real(atof(token.c_str())));
	      }
	    else if (is_identifier(token))
	      {
		    tree_stack.push(new Variable(token));
		  }
	      }
	    catch(...)
	      {
		while (! tree_stack.empty()) 
		  {
		    delete tree_stack.top();
		    tree_stack.pop();
		  }
		throw;
	      }
	  }
   
	// Det ska bara finnas ett tr�d p� stacken om postfixen �r korrekt.

	if (tree_stack.empty())
	  {
	    throw Expression_Error("inget postfix\n");

	  }

	if (tree_stack.size() > 1)
	  {
	    while (! tree_stack.empty())
	      {
		delete tree_stack.top();
		tree_stack.pop();
	      }
	    throw Expression_Error("felaktigt postfix\n");

	  }

	// Returnera tr�det.
	return tree_stack.top();
      }
  } // namespace

  /*
   * make_expression()
   */
  Expression make_expression(const string& infix)
  {
    return Expression(make_expression_tree(make_postfix(infix)));
  }

  Expression::Expression(const Expression& in_tree)
  {
    if (in_tree.empty())
      {
	tree = (in_tree.tree)->clone();
      }
    else 
      {
	tree = nullptr;
      }
  }

  void Expression::operator=(Expression in_tree)
  {
    if (tree)
      { 
	delete tree;
	tree = NULL;
	swap(in_tree);
      }
  }

  Expression::~Expression()
  {
    delete tree;
  }
