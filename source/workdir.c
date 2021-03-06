/* external libraries */
#include <ncurses.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

/* internal libraries */
#include "vec.h"
#include "mem.h"

/* internal headers */
#include "workdir.h"
#include "state.h"

bool is_dir(char* path) {
    struct stat s;
    return ((stat(path, &s) == 0) && (s.st_mode & S_IFDIR));
}

void populate_owner_info(File_T* p_file){
    struct stat s;
    if(stat(p_file->path, &s) == 0){
        p_file->uid = s.st_uid;
        p_file->gid = s.st_gid;
    } //else error
}

void workdir_free(void* p_wd);

WorkDir_T* workdir_new(char* path){
    WorkDir_T* wd = mem_allocate(sizeof(WorkDir_T), &workdir_free);
    wd->idx = 0;
    wd->path = path;
    mem_retain(wd->path);
    wd->vfiles = vec_new(0);
    workdir_ls(wd);
    return wd;
}

void workdir_free(void* p_wd){
    WorkDir_T* wd = (WorkDir_T*)p_wd;
    mem_release(wd->vfiles);
    mem_release(wd->path);
}

void file_free(void* p_vfile){
    File_T* p_file = (File_T*) p_vfile;
    //only free name if == special value '..'. else will be location in path
    if(strcmp(p_file->name, "..") == 0) mem_release(p_file->name);
    mem_release(p_file->path);
}

void workdir_set_idx(WorkDir_T* wd, int idx){
    frame_set_highlighting(state_get_focused_frame(), false, false);
    wd->idx = idx;
    if(idx < 0) wd->idx = 0;
    else if((unsigned int)idx >= vec_size(wd->vfiles))
        wd->idx = vec_size(wd->vfiles)-1;
    frame_set_highlighting(state_get_focused_frame(), true, true);
}

void workdir_next(WorkDir_T* wd) {
    workdir_set_idx(wd, wd->idx+1);
}

void workdir_prev(WorkDir_T* wd) {
    workdir_set_idx(wd, wd->idx-1);
}

void workdir_scroll_to_top(WorkDir_T* wd){
    workdir_set_idx(wd, 0);
}

void workdir_scroll_to_bot(WorkDir_T* wd){
    workdir_set_idx(wd, vec_size(wd->vfiles) - 1);
}

void workdir_cd(WorkDir_T* wd) {
    char* newpath = ((File_T*) vec_at(wd->vfiles, wd->idx))->path;
    if(is_dir(newpath)){
        mem_release(wd->path);
        wd->path = newpath;
        mem_retain(wd->path);
        wd->idx = 0;
    }
    workdir_ls(wd);
    state_set_refresh_state(REFRESH_CURR_WIN);
}

File_T* make_dotdot(char* path){
    File_T* dd = NULL;
    int last_slash = 0;
    if(strcmp(path, "/") != 0){
        dd = mem_allocate(sizeof(File_T), &file_free);
        dd->expanded = false;
        dd->name = mem_allocate(sizeof(char)*3, NULL);
        strcpy(dd->name, "..");
        for(int i=0; path[i] != 0; i++){
            if(path[i] == '/') last_slash = i;
        }
        if(last_slash == 0){
            dd->path = mem_allocate(sizeof(char)*2, NULL);
            strcpy(dd->path, "/");
        } else {
            dd->path = mem_allocate(sizeof(char)*(1+last_slash), NULL);
            strncpy(dd->path, path, last_slash);
            dd->path[last_slash] = 0;
        }
    }
    return dd;
}

char* ls_command(char* path){
    char* cmd = mem_allocate(sizeof(char) * (4+(strlen(path))), NULL);
    strcpy(cmd, "ls ");
    strcat(cmd, path);
    return cmd;
}

void workdir_ls(WorkDir_T* wd){
    File_T* dd = make_dotdot(wd->path);
    char* cmd = ls_command(wd->path);
    size_t len = 0; //unused. reflects sized allocated for buffer (filename) by getline
    ssize_t read;
    char* filename = 0;
    FILE* ls;
    int pathlength = strlen(wd->path);
    //free old file vector
    if(wd->vfiles) mem_release(wd->vfiles);
    //open new ls pipe
    ls = popen(cmd, "r");
    //initialize new file vector
    wd->vfiles = vec_new(0);
    if(dd) vec_push_back(wd->vfiles, dd);
    while ((read = getline(&filename, &len, ls)) != -1){
        File_T* file = mem_allocate(sizeof(File_T), &file_free);
        file->path = mem_allocate((pathlength+read+1)*sizeof(char), NULL);
        int filename_offset = pathlength;
        filename[read-1]=0; //remove ending newline
        //build full path:
        strcpy(file->path, wd->path);
        if (wd->path[pathlength-1] != '/') {
            strcat(file->path, "/");
            filename_offset += 1;
        }
        strcat(file->path, filename);
        file->name = &(file->path[filename_offset]);
        populate_owner_info(file);
        file->expanded = false;
        vec_push_back(wd->vfiles, file);
    }
    free(filename);
    pclose(ls);
    mem_release(cmd);
}

void workdir_seek(WorkDir_T* wd, char* search){
    unsigned int i = 0;
    if(strcmp(((File_T*)vec_at(wd->vfiles, 0))->name, "..") == 0) i++;
    while(i < vec_size(wd->vfiles) && strcmp(((File_T*)vec_at(wd->vfiles, i))->name, search) < 0) i++;
    workdir_set_idx(wd, i);
}

void workdir_expand_selected(WorkDir_T* wd){
    ((File_T*)vec_at(wd->vfiles, wd->idx))->expanded = true;
    state_set_refresh_state(REFRESH_CURR_WIN);
}
void workdir_collapse_selected(WorkDir_T* wd){
    ((File_T*)vec_at(wd->vfiles, wd->idx))->expanded = false;
    state_set_refresh_state(REFRESH_CURR_WIN);
}

