#include "shelpers.hpp"

/*
  text handling functions
 */

bool splitOnSymbol(std::vector<std::string>& words, int i, char c){
  if(words[i].size() < 2){ return false; }
  int pos;
  if((pos = words[i].find(c)) != std::string::npos){
	if(pos == 0){
	  //starts with symbol
	  words.insert(words.begin() + i + 1, words[i].substr(1, words[i].size() -1));
	  words[i] = words[i].substr(0,1);
	} else {
	  //symbol in middle or end
	  words.insert(words.begin() + i + 1, std::string(1,c));
	  std::string after = words[i].substr(pos + 1, words[i].size() - pos - 1);
	  if(!after.empty()){
		words.insert(words.begin() + i + 2, after);
	  }
	  words[i] = words[i].substr(0, pos);
	}
	return true;
  } else {
	return false;
  }

}

std::vector<std::string> tokenize(const std::string& s){

  std::vector<std::string> ret;
  int pos = 0;
  int space;
  //split on spaces
  while((space = s.find(' ', pos)) != std::string::npos){
	std::string word = s.substr(pos, space - pos);
	if(!word.empty()){
	  ret.push_back(word);
	}
	pos = space + 1;
  }

  std::string lastWord = s.substr(pos, s.size() - pos);
  if(!lastWord.empty()){
	ret.push_back(lastWord);
  }

  for(int i = 0; i < ret.size(); ++i){
      char ch[] = {'&', '<', '>', '|', '$', '=', '/'};
	for(auto c : ch){
	  if(splitOnSymbol(ret, i, c)){
		--i;
		break;
	  }
	}
  }
  
  return ret;
  
}


std::ostream& operator<<(std::ostream& outs, const Command& c){
  outs << c.exec << " argv: ";
  for(const auto& arg : c.argv){ if(arg) {outs << arg << ' ';}}
  outs << "fds: " << c.fdStdin << ' ' << c.fdStdout << ' ' << (c.background ? "background" : "");
  return outs;
}

//returns an empty vector on error
/*

  You'll need to fill in a few gaps in this function and add appropriate error handling
  at the end.

 */
std::vector<Command> getCommands(const std::vector<std::string>& tokens){
  std::vector<Command> ret(std::count(tokens.begin(), tokens.end(), "|") + 1);  //1 + num |'s commands

  int first = 0;
  int last = std::find(tokens.begin(), tokens.end(), "|") - tokens.begin();
  bool error = false;
  for(int i = 0; i < ret.size(); ++i){
	if((tokens[first] == "&") || (tokens[first] == "<") ||
		(tokens[first] == ">") || (tokens[first] == "|")){
	  error = true;
	  break;
	}
    
	ret[i].exec = tokens[first];
	ret[i].argv.push_back(tokens[first].c_str()); //argv0 = program name
	std::cout << "exec start: " << ret[i].exec << std::endl;
	ret[i].fdStdin = 0;
	ret[i].fdStdout = 1;
	ret[i].background = false;
    ret[i].setEV = false;
    ret[i].getEV = false;
    
	for(int j = first + 1; j < last; ++j){
	  if(tokens[j] == ">" || tokens[j] == "<" ){
		//I/O redirection
          
          // if it's >, open the fd on its fdStdout with write access
          if(tokens[j] == ">"){
              ret[i].fdStdout = open(tokens[j+1].c_str(),O_WRONLY|O_CREAT|O_TRUNC, 0644);
              if(ret[i].fdStdout == -1){
                  perror("Fail to open file");
                  error = true;
              }
          }
          else{// if it's <, open the fd on its fdStdin with read access
              ret[i].fdStdin = open(tokens[j+1].c_str(), O_RDONLY);
              if(ret[i].fdStdin == -1){
                  perror("Fail to open file");
                  error = true;
              }
          }
          
          break;

          
      }else if(tokens[j] == "="){
          // set environment variable
          if(setenv(tokens[j-1].c_str(),tokens[j+1].c_str(),0 ) == -1){
              perror("set environment variable fail");
              error = true;
          }
          ret[0].setEV = true; // don't fork child
          
      }else if(tokens[j] == "$"){
          // get environment variable
          char* ev = getenv(tokens[j+1].c_str());
          if(ev == NULL){
              perror("no such environment variable");
          }
          ret[i].argv.push_back(ev);
          ret[i].getEV = true; // skip next run
          
      }else if(tokens[j] == "&"){
		//Fill this in if you choose to do the optional "background command" part
          ret[i].background = true;
          
//		assert(false);
	  } else {
		//otherwise this is a normal command line argument!
          // if it has getten ev, skip one run
          if(ret[i].getEV){
              ret[i].getEV = false;
              continue;
          }
          else{
              ret[i].argv.push_back(tokens[j].c_str());
          }
	  }
	  
	}
	if(i > 0){
	  /* there are multiple commands.  Open open a pipe and
		 Connect the ends to the fds for the commands!
	  */
        //create pipe and connect the output of the first command as input to the second command.
        int p[2];
        if(pipe(p) == -1){
            perror("Fail on open pipe");
            error = true;
        }
        ret[i].fdStdin = p[0];
        ret[i-1].fdStdout = p[1];
//	  assert(false);
	}
	//exec wants argv to have a nullptr at the end!
	ret[i].argv.push_back(nullptr);

	//find the next pipe character
	first = last + 1;
	if(first < tokens.size()){
	  last = std::find(tokens.begin() + first, tokens.end(), "|") - tokens.begin();
	}
  }

  if(error){
	//close any file descriptors you opened in this function!
      for(Command cmd : ret){
          if(cmd.fdStdout != 1){
              if(close(cmd.fdStdout) == -1){
                  perror("close fd fail");
              }
          }
          if(cmd.fdStdin != 0){
              if(close(cmd.fdStdin) == -1){
                  perror("close fd fail");
              }
          }
      }
      ret.clear();
  }
  return ret;
}

std::string currentPath(){
    char path[255];
    getcwd(path, sizeof(path));
    std::vector<std::string> pathv = tokenize(path);
    return pathv[pathv.size()-1];
}
