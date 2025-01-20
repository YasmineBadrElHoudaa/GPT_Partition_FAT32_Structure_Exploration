#include <glob.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#define TAILLE_SECTEUR 512
#define ERR while(1){printf("Error : %s\n",strerror(errno));exit(1);}
#pragma pack(push, 1)

// Structure pour stocker les informations d'une entrée de partition GPT
typedef struct {
    uint8_t  type_guid[16];
    uint8_t  partition_guid[16];
    uint64_t first_lba;
    uint64_t last_lba;
    uint64_t attributes;
    uint16_t name[36];
} GptPartitionEntry;

// Structure pour stocker les informations des entrées de répertoire FAT32
typedef struct {
    char name[12];          // Nom de fichier en format court
    uint32_t size;          // Taille en octets
    uint32_t firstCluster;  // Numéro du premier cluster
} Fat32DirEntry;

#pragma pack(pop)

// Fonction pour lire les informations d'une entrée de répertoire FAT32
void read_fat32_dir_entry(FILE *fp, long offset, Fat32DirEntry *entry) {
    uint8_t buffer[32];
    fseek(fp, offset, SEEK_SET);
    fread(buffer, 1, 32, fp);

    // Lecture du nom en format court
    memcpy(entry->name, buffer, 11);
    entry->name[11] = '\0';

    // Lecture de la taille
    entry->size = buffer[28] | (buffer[29] << 8) | (buffer[30] << 16) | (buffer[31] << 24);

    // Lecture du premier cluster
    uint16_t high_cluster = buffer[20] | (buffer[21] << 8);
    uint16_t low_cluster = buffer[26] | (buffer[27] << 8);
    entry->firstCluster = (high_cluster << 16) | low_cluster;
}

void Lire_secteur(FILE* disque, unsigned int num_secteur, char* buffer)
{
	if (fseek(disque, num_secteur*TAILLE_SECTEUR, SEEK_SET) == -1)
		ERR;
	if (fread(buffer, TAILLE_SECTEUR, 1, disque) == -1)
		ERR;
}

void Afficher_secteur(FILE* disque, unsigned int num_secteur)
{
	char buffer[TAILLE_SECTEUR];
	Lire_secteur(disque, num_secteur, buffer);
	for (size_t addr = 0; addr < TAILLE_SECTEUR/16; addr += 1)
	{
		printf("%08x : ", addr*16);
		for (size_t i = 0; i < 16; i++)
			printf("%02X ", (unsigned char) buffer[addr*16+i]);
		printf("\n");
	}
}
void Lire_secteur_Niem_oct(FILE* disque, unsigned int num_secteur,unsigned int Num_octet, unsigned int nb_octet, char* buffer)
{
	if (fseek(disque, num_secteur*TAILLE_SECTEUR+Num_octet, SEEK_SET) == -1)
		ERR;
	if (fread(buffer, nb_octet, 1, disque) == -1)
		ERR;
}
void Lire_secteur_Num_pos(FILE* disque, unsigned int Num_octet, unsigned int nb_octet, char* buffer)
{
	if (fseek(disque, Num_octet, SEEK_SET) == -1)
		ERR;
	if (fread(buffer, nb_octet, 1, disque) == -1)
		ERR;
}

void TP_Syc2(FILE* disque,FILE* fat)
{
	unsigned char secteur_partition[TAILLE_SECTEUR];
	char secteur_boot[TAILLE_SECTEUR];
	unsigned char buff[TAILLE_SECTEUR];
	int32_t num_partitions;
	// Lire le secteur du Partition table header de GPT
	Lire_secteur(disque, 1, secteur_partition);
	Lire_secteur(fat, 1, secteur_boot);
	
	
	(Lire_secteur_Niem_oct(disque,1,0x54,4,buff) ); // Taille d'une entrée dans la table de partitions
	int32_t taille_entree_partition = *(int32_t*)buff; //taille entree 
	(Lire_secteur_Niem_oct(disque,1,0x48,4,buff)); // Premier LBA de la table de partitions
	int32_t partition_table_start = *(int32_t*)buff ;
	
	int32_t fat32_lba; // le LBA de début de la deuxième partition
	
	// Considerons que TAILLE_SECTEUR est divisible par taille_entree_partition
	
	int32_t cur_partition = 0; 


		// Lire le secteur qui va contenir (TAILLE_SECTEUR / taille_entree_partition) entrées
		Lire_secteur(disque, partition_table_start + cur_partition * taille_entree_partition / TAILLE_SECTEUR, secteur_partition);
		// Commençons par la première entrée au début du buffer 
		char* partition_entry_buffer = (char*) secteur_partition;
		int64_t lba_debut, lba_fin;
		
		int64_t num = partition_table_start + cur_partition * taille_entree_partition / TAILLE_SECTEUR ;
		// Pour chaque entrée du buffer lu (qui contient (TAILLE_SECTEUR / taille_entree_partition) entrées)
		
			(Lire_secteur_Niem_oct(disque,num ,taille_entree_partition +0x20,8,buff) );
			lba_debut = *(int64_t*)buff;
			 (Lire_secteur_Niem_oct(disque,num ,taille_entree_partition+0x28,8,buff));
			 lba_fin = *(int64_t*)buff ;
			int64_t taille_secteurs = (lba_fin - lba_debut + 1);/* taille en secteurs */
			
			// Verifier si la partition est non utilisée (Partition type GUID == 0)
			
			 fat32_lba = lba_debut;
			// Déplacer le buffer pour traiter la prochaine entrée dans le secteur lu
			partition_entry_buffer += taille_entree_partition;
			
		
	
	printf("--------------------------------------------------------------------\n");
	printf("Lba de début de la partition fat32: %9d\n", fat32_lba); 
	printf("--------------------------------------------------------------------\n");
	printf("nombre de secteurs de la partition fat32: %13d \n",taille_secteurs );
	printf("--------------------------------------------------------------------\n");
	
	// Lire le secteur boot de la partition fat32
	Lire_secteur(fat, 1, secteur_boot);
	
	// Recupérer des champs de ce dérnier
	Lire_secteur_Niem_oct(disque,fat32_lba,44,4,buff);
	int32_t cluster_du_root = *(int32_t*)buff;
	Lire_secteur_Niem_oct(disque,fat32_lba,13,1,buff);
	int8_t secteurs_par_cluster = *(int8_t*)buff;// taille cluster en secteur
	
	Lire_secteur_Niem_oct(disque,fat32_lba,11,2,buff) ;
	int16_t bytes_par_secteur = *(int16_t*)buff;// nb octect par secteur
	Lire_secteur_Niem_oct(disque,fat32_lba,14,2,buff);
	int16_t secteurs_reserves = *(int16_t*)buff ;
	Lire_secteur_Niem_oct(disque,fat32_lba,16,1,buff);
	int8_t nombre_de_fats = *(int8_t*)buff;//nb fat
	Lire_secteur_Niem_oct(disque,fat32_lba,36,4,buff);
	int32_t secteurs_par_fat = *(int32_t*)buff;//sect fat
	// Calculer l'adresse LBA du premier cluster avec la formule donnée
	int32_t adresse_lba_premier_cluster = fat32_lba + secteurs_reserves + nombre_de_fats * secteurs_par_fat;
	
	
	printf("Le numero du premier cluster du repertoire racine: %u\n", cluster_du_root);
	printf("--------------------------------------------------------------------\n");

	printf("[+] Adresse LBA du premier cluster: %u\n", adresse_lba_premier_cluster);
	printf("--------------------------------------------------------------------\n");
	printf("Taille cluster en secteurs %13d \n  ",secteurs_par_cluster);
printf("--------------------------------------------------------------------\n");
}
int main()
{
	
	FILE* f = fopen("/dev/sdb", "rb");
	FILE* fat32 = fopen("/dev/sdb2", "rb");
	

	TP_Syc2(f,fat32);
	
	fclose(f);
	// Partie B **********************************************************
	const char *disk = "/dev/sdb";          // Remplacer par le périphérique réel
    const char *partition = "/dev/sdb2";    // Partition FAT32 (deuxième partition)
    FILE *fp_disk = fopen(disk, "rb");

    if (!fp_disk) {
        perror("Erreur lors de l'ouverture du fichier");
        return 1;
    }

    // Lire le secteur GPT header (LBA 1)
    fseek(fp_disk, 512, SEEK_SET);  // LBA 1 commence à 512 octets (1 secteur)
    uint8_t gpt_header[92]; // Taille minimale pour lire le header GPT
    fread(gpt_header, 1, 92, fp_disk);

    // Nombre d'entrées de partition et taille de chaque entrée
    uint32_t num_entries = *(uint32_t *)&gpt_header[80];
    uint32_t entry_size = *(uint32_t *)&gpt_header[84];
    uint64_t partition_table_lba = *(uint64_t *)&gpt_header[72];

    // Lire la table des partitions
    GptPartitionEntry *partitions = malloc(num_entries * entry_size);
    fseek(fp_disk, partition_table_lba * 512, SEEK_SET);
    fread(partitions, entry_size, num_entries, fp_disk);

    // Obtenir les informations de la deuxième partition
    uint64_t partition_start_lba = partitions[1].first_lba;

    free(partitions);

    printf("Partition FAT32 LBA de départ: %lu\n", partition_start_lba);

    // Ouvrir la partition FAT32
    FILE *fp_partition = fopen(partition, "rb");
    if (!fp_partition) {
        perror("Erreur lors de l'ouverture de la partition");
        fclose(fp_disk);
        return 1;
    }

    // Lecture des informations du boot sector de la partition FAT32
    uint8_t boot_sector[512];
    fread(boot_sector, 1, 512, fp_partition);

    uint16_t bytes_per_sector = boot_sector[11] | (boot_sector[12] << 8);
    uint8_t sectors_per_cluster = boot_sector[13];
    uint16_t reserved_sectors = boot_sector[14] | (boot_sector[15] << 8);
    uint8_t num_fats = boot_sector[16];
    uint32_t sectors_per_fat = boot_sector[36] | (boot_sector[37] << 8) | (boot_sector[38] << 16) | (boot_sector[39] << 24);
    uint32_t root_cluster = boot_sector[44] | (boot_sector[45] << 8) | (boot_sector[46] << 16) | (boot_sector[47] << 24);

    // Calcul de l'adresse LBA de la première entrée du répertoire racine
    uint32_t root_dir_lba = partition_start_lba + reserved_sectors + (num_fats * sectors_per_fat);

    printf("Root Directory LBA: %u\n", root_dir_lba);

    // Positionner sur le début du répertoire racine
    fseek(fp_partition, (root_dir_lba - partition_start_lba) * bytes_per_sector, SEEK_SET);

    // Lecture et affichage des entrées du répertoire racine
    long offset = 0; // Début du répertoire racine
    printf("|-----------|-----------|-----------|---------------|--------------|\n| Nom| TAILLE EN SECTEUR | TAILLE EN KO  | PREMIER CLUSTER | LBA PREMIER CLUSTER|\n|---------------------|-----------|-----------|---------------|--------------|\n");
    while (1) {
        Fat32DirEntry entry;
        read_fat32_dir_entry(fp_partition, offset, &entry);

        // Arrêter à la fin du répertoire
        if (entry.name[0] == 0x00) {
            break;
        }

        // Afficher les informations de l'entrée si elle n'est pas supprimée
        if ((entry.name[0] != 0xE5) && (entry.name[0] != 0x2E)) {
            // Calcul du LBA du premier cluster
            uint32_t lba_first_cluster = partition_start_lba + reserved_sectors + (num_fats * sectors_per_fat) + ((entry.firstCluster - 2) * sectors_per_cluster);

            // Calcul de la taille en secteurs et en KO
            uint32_t size_in_sectors = (entry.size + bytes_per_sector - 1) / bytes_per_sector;
            float size_in_kb = entry.size / 1024.0;

            // Affichage des informations demandées
            printf("| %s     | %u    | %.2f | %u   | %u   |\n", entry.name, size_in_sectors, size_in_kb, entry.firstCluster, lba_first_cluster);
            /*printf("Nom: %s\n", entry.name);
            printf("Taille (secteurs): %u\n", size_in_sectors);
            printf("Taille (KO): %.2f\n", size_in_kb);
            printf("Premier Cluster: %u\n", entry.firstCluster);
            printf("LBA du Premier Cluster: %u\n\n", lba_first_cluster);*/
        }

        // Passer à la prochaine entrée (32 octets par entrée)
        offset += 32;
    }

    // Fermeture des fichiers
    fclose(fp_disk);
    fclose(fp_partition);
}

