#include <sys/stat.h>
#include <string>
#include <fstream>
#include <iostream>
#include <git2.h>
using namespace std;


int main (int argc, char** argv)
{
  git_libgit2_init();

  // Create repository directory.
  string directory = "repository";
  mkdir (directory.c_str(), 0777);

  // Initialize the repository: git init.
  git_repository *repo = NULL;
  int result = git_repository_init (&repo, directory.c_str(), false);
  if (result != 0) cerr << giterr_last ()->message << endl;

  // Store two files in the repository directory.
  ofstream file;
  file.open ("repository/file1", ios::binary | ios::trunc);
  file << "Contents of file one123";
  file.close ();

  file.open ("repository/file2", ios::binary | ios::trunc);
  file << "Contents of file two";
  file.close ();

  // Run the equivalent of "git add ."

  // Get the git index.
  git_index * index = NULL;
  result = git_repository_index (&index, repo);
  if (result != 0) cerr << giterr_last ()->message << endl;

  // Add all files to the git index.
  result = git_index_add_all (index, NULL, 0, NULL, NULL);
  if (result != 0) cerr << giterr_last ()->message << endl;

  // Write the index to disk.
  result = git_index_write (index);
  if (result != 0) cerr << giterr_last ()->message << endl;

  // Run the equivalent of "git commit -a -m "commit message".
  git_oid tree_oid;
  git_index_write_tree(&tree_oid, index);
  
  git_signature *signature;
  git_signature_new(&signature, "nulltoken", "user@email.com", 1323847743, 60);

  git_tree *tree;
  git_tree_lookup(&tree, repo, &tree_oid);

  git_buf buffer;
  memset(&buffer, 0, sizeof(git_buf));

  git_oid commit_oid;

  if (git_repository_head_unborn(repo)) {
    git_message_prettify(&buffer, "Initial commit", 0, '#');
    git_commit_create_v(
      &commit_oid, // Commit SHA
      repo,        // Repo to store commit
      "HEAD",
      signature,
      signature,
      NULL,
      buffer.ptr,
      tree,
      0);
  }
  else {
    git_diff *diff = NULL;
    int diff_error = git_diff_index_to_workdir(&diff, repo, NULL, NULL);
    size_t diff_size = git_diff_num_deltas(diff);

    std::cout << diff_size << '\n';
//    if (diff_size >= 1) {

      /* Get HEAD as a commit object to use as the parent of the commit */
      git_oid parent_id;
      git_commit *parent;
      git_reference_name_to_id(&parent_id, repo, "HEAD");
      git_commit_lookup(&parent, repo, &parent_id);
      git_message_prettify(&buffer, "Actually New Changes", 0, '#');
    
      git_commit_create_v(
        &commit_oid, // Commit SHA
        repo,        // Repo to store commit
        "HEAD",      // Ref to update
        signature,   // Author
        signature,   // Committer
        NULL,        // UTF-8 encoding
        buffer.ptr,  // Commit message
        tree,        // Tree found from index
        1,           // Number of parents
        parent);     // Commiting one parent (if there is more than one, it needs to be an array)

        git_commit_free(parent);
//    }

    git_diff_free(diff);
  }

  git_buf_free(&buffer);
  git_signature_free(signature);
  git_tree_free(tree);

  // Run "git status" to see the result.
  system ("cd repository; git status");
  system ("cd repository; git log");

  // Free resources.
  git_index_free (index);
  git_repository_free (repo);
  git_libgit2_shutdown();

  return 0;
}
