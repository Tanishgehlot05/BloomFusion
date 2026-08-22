//the actual one
#include <iostream>
#include <vector>
#include <cmath>
#include <climits>
#include<bits/stdc++.h>

using namespace std;


class Node {
public:
    std::vector<long long> keys;
    std::vector<Node*> children;
    int key_count;

    bool isLeaf;
    int m;
    std::vector<int> b_bits;
    std::vector<int> m_bits;
    int gap;
    int node_sketch; // for sketching
    int mask_sketch;// for desketchifying
    int mask_q;  // bit mask
    
    int mask_b;
    int mask_bm;

    Node(int max_keys = 0) : key_count(0), isLeaf(true), m(0), gap(0), node_sketch(0), mask_sketch(0), mask_q(0), mask_b(0), mask_bm(0) {
        if (max_keys != 0) {
            keys.resize(max_keys + 1);
            children.resize(max_keys + 2);
        }
    }
};

class FusionTree {
private:
    int keys_max;
    int keys_min;
    long long w;
    Node* root;

 
    vector<int> getDiffBits(vector<long long> &keys) {
        vector<int> res;
        long long bits = 0;

        for (long long i = 0; i < keys.size(); i++) {
            if (keys[i] == 0) {
                break;
            }
            for (long long j = 0; j < i; j++) {
                long long w = this->w;
                // cout<<"(keys[i] & (1 << w)) "<<(keys[i])<<endl;
                // cout<<"(keys[j] & (1 << w)) "<<(keys[j])<<endl;
                
                while ((keys[i] & (1LL << w)) == (keys[j] & (1LL << w)) && w >= 0) {
                    w--;
                }
                if (w >= 0) {
                    // cout<<"bits "<<bits<<endl;
                    // cout<<"w "<<w<<endl;
                    bits = bits | (1LL << w);
                    // cout<<"bits "<<bits<<endl;
                }
             
            }
        }
        // cout<<"bits "<<bits<<endl;
        long long i = 0;
        long long x=0;
        map<int,int> mpp;
        while (i < this->w) {
            // cout<<"bits & (1 << i )"<<(bits & (1 << i))<<"\n";
            // cout<<"x "<<x<<"\n";
            if ((bits & (1LL << i) ) > 0 && mpp[bits & (1LL << i)]==0){
                res.push_back(i);
                x=(bits & (1LL << i));
                mpp[x]++;
                // cout<<"here "<<i<<endl;
            }
            i++;
        }
    
        // cout<<"res\n";
        // for(auto val:res) cout<<val<<" "; cout<<endl;
        return res;
    }
    pair<vector<int>, int> getConst(const std::vector<int>& b_bits) {
        int r = static_cast<int>(b_bits.size());
        vector<int> m_bits(r, 0);
        for (int t = 0; t < r; ++t) {
            long long mt = 0;
            bool flag = true;
            while (flag) {
                flag = false;
                for (int i = 0; i < r; ++i) {
                    if (flag) break;
                    for (int j = 0; j < r; ++j) {
                        if (flag) break;
                        for (int k = 0; k < t; ++k) {
                            if (mt == b_bits[i] - b_bits[j] + m_bits[k]) {
                                flag = true;
                                break;
                            }
                        }
                    }
                }
                if (flag) mt += 1;
            }
            m_bits[t] = mt;
        }

        long long m = 0;
        for (auto i : m_bits) {
            m |= (1LL << i);
        }
        return make_pair(m_bits, m);
    }

    int getMask(const std::vector<int>& mask_bits) {
        long long res = 0;
        for (auto i : mask_bits) {
            res |= (1LL << i);
        }
        return res;
    }

    void initiateNode(Node* node) {
        if (node->key_count != 0) {
            // cout<<"here"<<endl;-->right
            // for(auto val:node->keys) cout<<val<<" ";cout<<endl;
            node->b_bits =this-> getDiffBits(node->keys);
            // cout<<"here bit"<<endl;
            // for(auto val:node->b_bits) cout<<val<<" ";cout<<endl;

            std::pair<std::vector<int>, int> m_info = getConst(node->b_bits);
            node->m_bits = m_info.first;
            node->m = m_info.second;
            // cout<<"node->m "<<node->m<<endl;
            // cout<<"getMask(node->b_bits)"<<getMask(node->b_bits)<<endl;

            
            node->mask_b = getMask(node->b_bits);

            vector<int> temp;
            // cout<<"okk "<<node->b_bits.size()<<endl;
            for (int i = 0;i< (node->b_bits).size(); ++i) {
                temp.push_back(node->b_bits[i] + node->m_bits[i]);
                
            }
            
            // for(auto val:temp ) cout<<val<<" ";cout<<endl;
            node->mask_bm = getMask(temp);

            int r3 = static_cast<int>(pow(node->key_count, 3));
            node->node_sketch = 0;
            int sketch_len = r3 + 1;
            node->mask_q = 0;
            node->mask_sketch = 0;

           

            for (int i = 0; i < node->key_count; ++i) {
                int sketch = sketchApprox(node, node->keys[i]);

                // cout<<"sketch "<<sketch<<endl;

                long long temp = (1LL << r3) | sketch;
                node->node_sketch <<= sketch_len;
                node->node_sketch |= temp;
                node->mask_q |= (1LL << (i * sketch_len));
                node->mask_sketch |= ((1LL << (sketch_len - 1)) << (i * sketch_len));
            }
        }
    }

    int sketchApprox(Node* node, int x) {
        int xx = x & node->mask_b;
        // cout<<"node->mask_b "<<node->mask_b<<endl;
        // cout<<"xx "<<xx<<endl;
        int res = xx * node->m;
        // cout<<"node->mask_bm "<<node->mask_bm<<endl;
        res = res & node->mask_bm;
        // cout<<"res "<<res<<endl;
        return res;
    }

public:
    FusionTree(int word_len=64 , double c = 1.0/5.0) {
        int t;
        t = static_cast<int>(pow(word_len, c));
        keys_max=2*t-1;
    
        w=word_len;
        keys_min = t-1;
        root = new Node(keys_max);
        root->isLeaf = true;
    }






void splitChild(Node* node, int x) {
    // A B-tree split function. Splits the child of the node at index x.
    Node* z = new Node(this->keys_max);
    Node* y = node->children[x]; // y is to be split

    // Position of the key to propagate
    int pos_key = (this->keys_max / 2);

    z->key_count = this->keys_max - pos_key - 1;

    // Insert the first half of the keys into z
    for (int i = 0; i < z->key_count; ++i) {
        z->keys[i] = y->keys[pos_key + i + 1];
        y->keys[pos_key + i + 1] = 0;
    }

    if (!y->isLeaf) {
        for (int i = 0; i < z->key_count + 1; ++i) {
            z->children[i] = y->children[pos_key + i + 1];
        }
    }

    y->key_count = this->keys_max - z->key_count - 1;

    // Insert the key into the node
    node->keys[x] = y->keys[pos_key];

    // Achieve the same effect as shifting all keys after setting pos_key to nullptr
    y->keys.erase(y->keys.begin() + pos_key);
    y->keys.push_back(0);

    // Insert z as a child at position x + 1
    node->children.insert(node->children.begin() + x + 1, z);

    node->key_count += 1;
}

void insertNormal(Node* node, int k) {
    // Insert k into node when there is no chance of splitting the root
    if (node->isLeaf) {
        int i = node->key_count;
        while (i >= 1 && k < node->keys[i - 1]) {
            node->keys[i] = node->keys[i - 1];
            i -= 1;
        }
        node->keys[i] = k;
        node->key_count += 1;
    } else {
        int i = node->key_count;
        while (i >= 1 && k < node->keys[i - 1]) {
            i -= 1;
        }

        // i is the position of the appropriate child

        if (node->children[i]->key_count == this->keys_max) {
            splitChild(node, i);
            if (k > node->keys[i]) {
                i += 1;
            }
        }
        insertNormal(node->children[i], k);
    }
}

void insert(int k) {
    // This insert checks if splitting is needed
    // then it splits and calls normalInsert

    // If the root needs splitting, a new node is assigned as root
    // with split nodes as children
    if (root->key_count == keys_max) {
        Node* temp_node = new Node(keys_max);
        temp_node->isLeaf = false;
        temp_node->key_count = 0;
        temp_node->children[0] = root;
        root = temp_node;
        splitChild(temp_node, 0);
        insertNormal(temp_node, k);
    } else {
        insertNormal(root, k);
    }
}


int parallelComp(Node* node, int k) {

    // This function should basically give the index such
    // that sketch of k lies between 2 sketches
    
    int sketch = sketchApprox(node, k);

    // cout<<"here->sketch "<<sketch<<endl;
    // This will give repeated sketch patterns to allow for comparison
    // in const time
    long long sketch_long = 1ll*sketch * node->mask_q;

    long long res = node->node_sketch - sketch_long;

    // Mask out unimportant bits
    res &= node->mask_sketch;

    // Find the leading bit. This leading bit will tell position i of
    // such that sketch(keyi-1) < sketch(k) < sketch(keyi)
    
    long long i = 0;
    // cout<<"res "<<res<<endl;
    while ((1LL << i) < res) {
         // This is the part where one should focus as res can be >10^18 
        // that means it is not possible to calculate in standard arithemtic operation in c++
        // we can do same in python or similar languages

        i += 1;
        // cout<<"i "<<i<<endl;
    }
    i += 1;
    long long sketch_len = pow(node->key_count, 3) + 1;

    // cout<<"sketch_len "<<sketch_len<<endl;
    // cout<<"i "<<i<<endl;
    // cout<<"node->key_count "<<node->key_count<<endl;
    
    return node->key_count - (i / sketch_len);
}

void inOrderTraversal(Node* node, function<void(int)> visit) {
    if (node == nullptr) {
        return;
    }

    if (node->isLeaf) {
        for (int i = 0; i < node->key_count; i++) {
            visit(node->keys[i]);
        }
    } else {
        for (int i = 0; i < node->key_count; i++) {
            inOrderTraversal(node->children[i], visit);
            visit(node->keys[i]);
        }
        inOrderTraversal(node->children[node->key_count], visit);
    }
}

void inOrderTraversal(function<void(int)> visit) {
    inOrderTraversal(root, visit);
}
int successor(int k, Node* node=nullptr) {
    if (node == nullptr) {
        node = this->root;
    }

    if (node->key_count == 0) {
        // cout<<"here1"<<endl;
        if (node->isLeaf) {
            return -1;
        } else {
            return successor(k, node->children[0]);
        }
    }

    // The corner cases are not concretely defined.
    // Another alternative to handle these would be to have
    // -inf and inf at the corners of the keys array
    // cout<<"here--count->"<<node->key_count<<endl;
    // cout<<"here--11->"<<node->keys[0]<<endl;
    if (node->keys[0] >= k) {
        // cout<<"here2"<<endl;
        if (!node->isLeaf) {
            int res = successor(k, node->children[0]);
            if (res == -1) {
                return node->keys[0];
            } else {
                return min(node->keys[0]*1ll, res*1ll);
            }
        } else {
            return node->keys[0];
        }
    }
    // cout<<"here12--->"<<node->keys[node->key_count -1]<<endl;
    if (node->keys[node->key_count - 1] < k) {
        // cout<<"here3"<<endl;
        if (node->isLeaf) {
            return -1;
        } else {
            return successor(k, node->children[node->key_count]);
        }
    }

    int pos = parallelComp(node, k);
    // std::cout << "pos = " << pos << std::endl;
    
  

    if (pos >= node->key_count) {
        // cout<<"here4"<<endl;
        // std::cout << "pos >= key_count" << std::endl;
        // Handle the case where pos is out of bounds
        // You can add your own error handling logic here
    }

    if (pos == 0) {
        // cout<<"here5"<<endl;
        pos += 1;
    }
    // cout<<"here-pos->"<<pos<<endl;
    // Find the common prefix
    int x = std::max(node->keys[pos - 1], node->keys[pos]);
    int common_prefix = 0;
    long long i = this->w;
    while (i >= 0 && (x & (1LL << i)) == (k & (1LL << i))) {
        common_prefix |= x & (1LL << i);
        i--;
    }

    if (i == -1) {
        return x;
    }
    // cout<<"i "<<i<<endl;
    int temp = common_prefix | (1LL << i);
    // cout<<"temp"<<temp<<endl;
    pos = parallelComp(node, temp);

    //   cout<<"here-pos->"<<pos<<endl;


    if (node->isLeaf) {
        return node->keys[pos];
    } else {
        int res = successor(k, node->children[pos]);
        if (res == -1) {
            return node->keys[pos];
        } else {
            return res;
        }
    }
}
int predecessor(int k, Node* node = nullptr) {
    if (node == nullptr) {
        node = this->root;
    }

    if (node->key_count == 0) {
        if (node->isLeaf) {
            return -1;
        } else {
            return predecessor(k, node->children[0]);
        }
    }

    if (node->keys[0] > k) {
        if (!node->isLeaf) {
            return predecessor(k, node->children[0]);
        } else {
            return -1;
        }
    }

    if (node->keys[node->key_count - 1] <= k) {
        if (node->isLeaf) {
            return node->keys[node->key_count - 1];
        } else {
            int ret = predecessor(k, node->children[node->key_count]);
            return max(ret*1ll, node->keys[node->key_count - 1]*1ll);
        }
    }

    int pos = parallelComp(node, k);
    // cout<<"pos "<<pos<<endl;
    if (pos >= node->key_count) {
        // cout<< node->keys << " " << pos << " ERROR? pos > key_count" << endl;
       
        string dump;
        cin >> dump;
    }

    if (pos == 0) {
        pos += 1;
    }
    // cout<<"pos "<<pos<<endl;

    int x = node->keys[pos];
    int common_prefix = 0;
    long long i = this->w;
    while (i >= 0 && ((x & (1LL << i)) == (k & (1LL << i)))) {
        common_prefix |= x & (1LL << i);
        i -= 1;
    }
    if (i == -1) {
        return x;
    }

    int temp = common_prefix | ((1LL << i) - 1);
    pos = parallelComp(node, temp);
    if (pos == 0) {
        if (node->isLeaf) {
            return node->keys[pos];
        }
        int res = predecessor(k, node->children[1]);
        if (res == -1) {
            return node->keys[pos];
        } else {
            return res;
        }
    }

    if (node->isLeaf) {
        return node->keys[pos - 1];
    } else {
        int res = predecessor(k, node->children[pos]);
        if (res == -1) {
            return node->keys[pos - 1];
        } else {
            return res;
        }
    }
}

void initiate(Node* node=nullptr) {
    if (node == nullptr) {
        node = new Node(this->keys_max);
    }
    initiateNode(node);
    if (!node->isLeaf) {
        for (int i = 0; i < this->keys_max + 1; i++) {
            initiate(node->children[i]);
        }
    }
}

void initiateTree() {
    initiate(this->root);
}


};

int main() {
    // Create a Fusion Tree with degree 2
    FusionTree ft; // Create a FusionTree with default parameters
    ft= FusionTree(32);
    vector<int> vi;
    cout<<"Press 1 for see the inbuilt output and 2 for user interaction input\n";
    int p;
    cin>>p;
    if(p==1){
    cout<<"These are sample example of implementing Fusion tree\n";
    cout<<"*******************************************************\n";
    
    // Insert some keys
    for (int i = 1; i <= 5; i++) {
        vi.push_back(i * i);
    }
    for (int i = 0; i <= vi.size()-1; i++) {
        ft.insert(vi[i]);
    }
    ft.initiateTree();
    auto visit = [](int key) {
        cout << key << " ";
    };
    cout<<"Application of Fusion Tree\n";
     cout << "1) Fusion Tree size: " ;
     cout << vi.size() << endl;
    cout << "2) After Sorting : ";
    ft.inOrderTraversal(visit);
    cout << endl;

    int idx1 = ft.predecessor(3);
    int idx2 = ft.predecessor(16);
    int idx3 = ft.predecessor(0);
    int idx4 = ft.successor(0);
    int idx5 = ft.predecessor(10);
    cout << "3) Queried elements:" << endl;
    cout <<"predecessor of 3: "<<idx1 <<endl;
    cout<<"predecessor of 16: " <<idx2<<endl;
    cout<<"predecessor of 0: " <<idx3<<endl;
    cout<<"successor of 0: " <<idx4<<endl;
    cout<<"successor of 10: " <<idx5<<endl;
        
    cout<<"*******************************************************\n";

    }
    else{
        if(p==2){
    cout<<"Now its Your Turn to give input\n";
        cout<<"Enter the number of elements You wanted to Enter\n";
        int n;
        cin>>n;
        vi.clear();
        for (int i = 0; i < n; i++) {
            int x;
            cin>>x;
            vi.push_back(x);
            ft.insert(x);
        }
        ft.initiateTree();


    while(1){
        

        cout<<"1) Size of the Fusion tree\n ";
        cout<<"2) Sorting of the Fusion tree\n ";
        cout<<"3) Query of the Fusion tree like predecessor and successor\n ";
        cout<<"4) For Exit\n ";
        cout<<" [NOTE]:-These are very efficient with respect to time complexity for finding predecessor and successor in O(1)\n";
        int choice;
        cin>>choice;

        if(choice==1){
            cout<<"size "<<vi.size()<<endl;
        }
        if(choice==2){

            cout << "2) After Sorting : ";
            auto visit = [](int key) {
                cout << key << " ";
            };     
            ft.inOrderTraversal(visit);
            cout << endl;
            
        }
        if(choice==3){
            int k;
            cout<<"press 1 for predecessor 2 for successor \n"<<endl;
            cin>>k;
            if(k==1){
                    int data;
                    cin>>data;
                    int idxp = ft.predecessor(data);
                    cout<<idxp<<endl;
            }else{

                if(k==2){
                    int data;
                    cin>>data;
                    int idxs = ft.successor(data);
                    cout<<idxs<<endl;
                }

                else cout<<"You press wrong key\n";
            }
        }
        else break;

    }

        }
        else cout<<"You press the wrong key\n";
    }

    return 0;
}
