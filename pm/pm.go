package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
	"time"
)

type Program struct {
	Dir  string
	Name string
}

var programs = []Program{
	//{Dir: "../navmap", Name: "./navmap"},
	{Dir: "../lcd", Name: "./run.sh"},
}

var restart_wait_time = 1 * time.Second

func main() {
	out_dir := filepath.Join("out", time.Now().Format("2006-01-02_15-04-05"))
	err := os.MkdirAll(out_dir, 0755)
	if err != nil {
		log.Fatal("failed to create output directory:", err)
	}

	log_file, err := os.OpenFile(filepath.Join(out_dir, "timeline.log"), os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
	if err != nil {
		log.Fatal("failed to create timeline.log:", err)
	}
	defer log_file.Close()

	multi_log := io.MultiWriter(os.Stdout, log_file)
	logger := log.New(multi_log, "", 0)

	for _, prog := range programs {
		go manage_process(prog, out_dir, logger)
	}

	select {}
}

func manage_process(prog Program, out_dir string, logger *log.Logger) {
	pname := filepath.Base(prog.Name)
	log_file_name := filepath.Join(out_dir, pname + ".log")

	for {
		log_file, err := os.OpenFile(log_file_name, os.O_CREATE|os.O_WRONLY|os.O_APPEND, 0644)
		if err != nil {
			logger.Println(pname, "failed to create log file:", err)
			time.Sleep(restart_wait_time)
			continue
		}

		cmd := exec.Command(prog.Name)
		cmd.Dir = prog.Dir

		stdout_pipe, _ := cmd.StdoutPipe()
		stderr_pipe, _ := cmd.StderrPipe()

		log_pm(logger, log_file, "starting", pname)

		err = cmd.Start()
		if err != nil {
			logger.Println(pname, "failed to start:", err)
			log_file.Close()
			time.Sleep(restart_wait_time)
			continue
		}

		go copy_with_prefix(stdout_pipe, log_file, "[stdout] ")
		go copy_with_prefix(stderr_pipe, log_file, "[stderr] ")

		err = cmd.Wait()
		exit_msg := "nil"
		if err != nil {
			exit_msg = err.Error()
		}

		log_pm(logger, log_file, "process ", pname, "exit with:", exit_msg)

		log_file.Close()
		time.Sleep(restart_wait_time)
	}
}

func log_pm(logger *log.Logger, file *os.File, messages ...string) {
	timestamp := time.Now().Format("15:04:05.000")
	line := fmt.Sprintf("[%s][ -- pm] %s", timestamp, strings.Join(messages, " "))
	logger.Println(line)
	fmt.Fprintln(file, line)
}


func copy_with_prefix(src io.Reader, dst io.Writer, prefix string) {
	scanner := bufio.NewScanner(src)
	for scanner.Scan() {
		timestamp := time.Now().Format("15:04:05.000")
		line := fmt.Sprintf("[%s]%s%s\n", timestamp, prefix, scanner.Text())
		dst.Write([]byte(line))
	}
}


