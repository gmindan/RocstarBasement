/* Generated by Together */

#include "Node.hpp"
#include "Face.hpp"
#include "Element.hpp"
#include "FaceList.hpp"
#include "ElementList.hpp"
#include "Mesh.hpp"

#include <assert.h>
#include <string.h>

Node::Node() : 
d_ID(-1),
d_flag(e_unset_flag),
d_nextNode (this),
d_faceArraySize(0),
d_numFaces(0),
d_elementArraySize(0),
d_numElements(0)
{}

Node::Node(MVec pos) :
d_position( pos ),
d_ID(-1),
d_flag(e_unset_flag),
d_nextNode (this),
d_faceArraySize(0),
d_numFaces(0),
d_elementArraySize(0),
d_numElements(0)
{}

Node::Node(MVec pos, int flag) :
d_position( pos ),
d_ID(-1),
d_flag(flag),
d_nextNode (this),
d_faceArraySize(0),
d_numFaces(0),
d_elementArraySize(0),
d_numElements(0)
{}

Node::~Node(){

  if( d_faceArraySize > 0 ){
    delete [] d_faces;
  }
  if( d_elementArraySize > 0 ){
    delete [] d_elements;
  }
}

void Node::addFace(Face * face){

  if( d_faceArraySize == d_numFaces ){
    d_faceArraySize =  (int)( d_faceArraySize * 1.25 + 3);
    Face **new_arr = new Face*[d_faceArraySize];
    if( d_numFaces > 0 ){
      memcpy(new_arr, d_faces, d_numFaces * sizeof(Face*) );
      delete [] d_faces;
    }
    d_faces = new_arr;
  }
  d_faces[d_numFaces] = face;
  d_numFaces++;
}

void Node::removeFace(Face * face){
  
  int i;
  int dif = 0;
  for( i = 0; i < d_numFaces; i++ ){
    if( (dif == 0) && (d_faces[i] == face) ){
      dif++;
      continue;
    }
    if( dif > 0 ){
      d_faces[i - dif] = d_faces[i];
    }
  }
  d_numFaces -= dif;
}

void Node::addElement(Element * elem){

  if( d_elementArraySize == d_numElements ){
    d_elementArraySize =  (int)( d_elementArraySize * 1.25 + 3);
    Element **new_arr = new Element*[d_elementArraySize];
    if( d_numElements > 0 ){
      memcpy(new_arr, d_elements, d_numElements * sizeof(Element*) );
      delete [] d_elements;
    }
    d_elements = new_arr;
  }
  d_elements[d_numElements] = elem;
  d_numElements++;
}


void Node::removeElement(Element * element){
  
  int i;
  int dif = 0;
  for( i = 0; i < d_numElements; i++ ){
    if( (dif == 0) && (d_elements[i] == element) ){
      dif++;
      continue;
    }
    if( dif > 0 ){
      d_elements[i - dif] = d_elements[i];
    }
  }
  d_numElements -= dif;
}

Face* Node::sharedFace(Node* n2, Node* n3, Node* n4 ){

  int i;
  for( i = 0; i < d_numFaces; i++ ){
    Node** fnodes = d_faces[i]->getNodes();
    int numn = d_faces[i]->getNumNodes();
    int count=0;
    int k;
    for( k = 0; k < numn; k++ ){
      if( fnodes[k] == n2 || fnodes[k] == n3 || fnodes[k] == n4){
	count++;
      }
    }
    if( ( n4 == 0 && count == 2) || count == 3 ){
      return d_faces[i];
    }
  }
  return 0;
}
 

void Node::addNextLink( Node* link ){

  Node* prev = this;

  while( prev->d_nextNode != this ){
    prev = prev->d_nextNode;
  }
  // now prev->next is this one
  prev->d_nextNode = link;
  prev = link;
  while( prev->d_nextNode != link ){
    prev = prev->d_nextNode;
  }
  // now prev->next is link
  prev->d_nextNode = this;

  prev = this;
  while( prev->d_nextNode != this ){
    prev = prev->d_nextNode;
  }
}


boolean Node::separate( Mesh* mesh, FaceList* list, int new_material ){

  //Use the facelist to get a list of involved elements, and see if they can
  //be separated or not.  This method should be called getSeparableElements. It
  //returns an element list with the involved elements if separable, empty if not
  //separable.
  ElementList* sep_elements = getSeparateElements( list );
  
  if( !sep_elements ){ //not a separable node.
    return FALSE;
  }

  //Its separable - make a new node at the same position, and add it to the mesh.
  //Note that it should have the same nodeflag as the initial node.
  Node *new_node = new Node(d_position, d_flag);
  mesh->addNode( new_node );

  //now we have to go back to the mesh class... kind of wierd design. 
  mesh->replaceNode(this, new_node, sep_elements, new_material );

  delete sep_elements;
 
  // now can call separate again for this & for new_node
  // recursion to separate further

  new_node->separate( mesh, list, new_material );
  separate( mesh, list, new_material);
  return TRUE;
}

ElementList* Node::getSeparateElements( FaceList* list ){

  // Collect ONLY regular faces - not in cohesive,
  // not boundary, not in the "list" ("list" is a facelist of
  // the faces involving the node that is trying to be separated). 
  
  //"list" is used as a separator list of faces.  We need to build a list of
  //faces that CAN be traversed. That list will be node_faces.
  
  FaceList node_faces;
  int i;
  for( i = 0; i < d_numFaces; i++ ){
  	//step through all the faces in the d_faces array for this node.
    Face *face = d_faces[i];
    //see if each face is on the "list" of separable faces sent in.
    if( list->move_to( face )){
      continue; //if it is, then go to the next face. 
    }
    
    //if this face is an exterior face, or if either element that it
    //is associated with is cohesive, then don't add it to the facelist. 
    if( !face->getElement2() || face->getElement1()->isCohesive() ||
	face->getElement2()->isCohesive() ){
      continue;
    }
    node_faces.insert_first( face );
  }
  
  //count the number of non-cohesive ("real") elements attached to this node. 
  int count_real=0;
  for( i = 0; i < d_numElements; i++ ){
    if( !d_elements[i]->isCohesive() ){
      count_real++;
    }
  }
  
  // Try to find ONE non-cohesive element.  Note the "break" in the "if" 
  // below if one is found!
  ElementList *shared_elements = new ElementList;
  for( i = 0; i < d_numElements; i++ ){
    if( !d_elements[i]->isCohesive() ){
      shared_elements->insert_first(d_elements[i]);
      break;
    }
  }
  
  //We should always find at least one non-cohesive element (says Alla). 
  if( shared_elements->size() != 1 ){
    // thoretically this should not happen (pure cohesive node)
    delete shared_elements;
    return 0;  //get out of town if node is pure cohesive - can't separate. 
  }

  // Since we found one non-cohesive element, now try to add more
  boolean changed = TRUE;
  while( changed ) { 
    changed = FALSE;
    //number of faces with non-cohesive or non-exterior elements on both sides. 
    int size = node_faces.size();  
    int i;
    //loop over all the candidate faces for this node. 
    for( i = 0; i < size; i++ ) { 
      //retrieve the list of faces for this node.
      Face* face = node_faces.get();  
      boolean found=FALSE;
      // check if face belongs to any shared element
      shared_elements->reset();
      int nums = shared_elements->size();
      int j;
      //for each element in the shared_elements list...
      for( j = 0; j < nums; j++ ) {
		Element* sh_el = shared_elements->get();
		Element* oelem(0);
		//check and see if the current face involves one of the elements on the list.
		//If so, get the other element that is on the other side of the face (put 
		//in oelem). 
		if( face->getElement1() == sh_el ){
	  		oelem = face->getElement2();
		}
		else if( face->getElement2() == sh_el ){
	  		oelem = face->getElement1();
		}
		//if find the second element ...
		if( oelem ){
	  		found = TRUE;
	  		//if it's not already on the shared elements list, then put it on the list. 
	  		if( !shared_elements->move_to( oelem ) ){
	    		shared_elements->insert_first( oelem );
	  		}
	  		break; //exit j loop (element loop)
		}
		
		//get ready to check the next element. 
		shared_elements->next();
		
      } //for (j=0...
      
      if( found ){ 
      	//remove the current face from the facelist
		node_faces.remove();
		//get ready to look at the new facelist (sans the current face)
		changed = TRUE;
		//exit the i loop and go back to the while (start over with new list of faces)
		break;
      }
      
      //one of the faces must be part of elements that are already on the list, so go on!
      node_faces.next();
      
    } //for (i=0 ...
  } //while(changed)
  
  // count_real is the number of non-cohesive elements attached to this node.
  // have separation surface
  //if the actual number of elements is greater than the number you found, then there must
  //be a separation plane.  If it is the same, then you went all the way around and found no
  //boundary.
  if( count_real > shared_elements->size() ){
    return shared_elements;
  }
  delete shared_elements;
  return 0;
}