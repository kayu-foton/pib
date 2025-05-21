
pm: pm.go
	go build -o pm pm.go

.PHONY: run
run: pm
	./pm

.PHONY: service
service: pm
	sudo cp ./pm.service /etc/systemd/system/
	sudo systemctl enable --now pm
